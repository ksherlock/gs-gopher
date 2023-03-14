
#pragma lint -1
#pragma optimize 79
#pragma noroot

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <gsos.h>
#include <window.h>


enum {
	BS_OK = 0,
	BS_USER_CANCELED,
	BS_NOT_BINSCII,
	BS_BAD_ALPHABET,
	BS_BAD_CRC,
	BS_TRUNCATED,
	BS_PARTIAL,
};


static int error(int e) {

	const char *messages[] = {
		"", "",
		"12/FiLeStArTfIlEsTaRt not found/^#0",
		"12/Bad alphabet/^#0",
		"12/Bad CRC/^#0",
		"12/BINSCII data truncated/^#0",
		"12/Multi-segment files are not supported/^#0",
	};

	if (e >= BS_NOT_BINSCII && e <= BS_PARTIAL)
		AlertWindow(awPointer, NULL, (Ref)messages[e]);
	return e;
}


extern unsigned crc_update(unsigned, void *, unsigned);
int FilePrompt(GSString255 *name, unsigned ftype, unsigned atype);


/*

FiLeStArTfIlEsTaRt
alphabet [64 bytes]
header [52 bytes]
data [64 bytes]
....
checksum [4 bytes]


alphabet = 
ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789()

(not hardcoded as some mailers changed paren characters)


*/

struct header {
	uint8_t file_size[3];
	uint8_t seg_start[3];
	uint8_t access;
	uint8_t file_type;
	uint8_t aux_type[2];
	uint8_t storage_type;
	uint8_t block_count[2];
	uint8_t create_date[2];
	uint8_t create_time[2];
	uint8_t mod_date[2];
	uint8_t mod_time[2];
	uint8_t seg_len[3];
	uint8_t crc[2];
	uint8_t reserved;
};

static uint8_t alphabet[256];

static int decode(const uint8_t *src, unsigned len, uint8_t *dest) {

	unsigned i;
	len >>= 2;
	for (i = 0; i < len; ++i) {
		unsigned c0 = alphabet[src[0]];
		unsigned c1 = alphabet[src[1]];
		unsigned c2 = alphabet[src[2]];
		unsigned c3 = alphabet[src[3]];

		// todo - if any are > 64, indicate a file error.

		dest[0] = (c3 << 2) | (c2 >> 4);
		dest[1] = (c2 << 4) | (c1 >> 2);
		dest[2] = (c1 << 6) | (c0);
		dest += 3;
		src += 4;
	}

	return (len << 1) + len;
}

static int de_scii(const uint8_t *ptr, long size) {
// ptr points to line AFTER FiLeStArTfIlEsTaRt

	static struct header header;
	static GSString32 filename;



	static uint8_t buffer[1536]; /* 1536 = 512 * 3 == 48 * 32 */

	static RefNumRecGS closeDCB = { 1, 0 };
	static IORecGS ioDCB = { 4, 0, buffer, 0, 0 };

	uint32_t file_size;
	uint32_t seg_start;
	uint32_t seg_len;


	unsigned i;
	unsigned c;
	unsigned crc;
	unsigned refNum;

	if (size < 64 + 52 + 4 + 2) {
		return BS_TRUNCATED;
	}

	memset(alphabet, 0xff, 256);
	for (i = 0; i < 64; ++i) {
		c = ptr[i];
		if (isspace(c)) return BS_BAD_ALPHABET;
		if (alphabet[c] != 0xff) return BS_BAD_ALPHABET;
		alphabet[c] = i;
	}

	for (i = 64; isspace(ptr[i]); ++i, --size);
	ptr += i;
	size -= i;

	if (size < 52 + 4) {
		return BS_TRUNCATED;
	}

	// name length
	filename.length = ptr[0] & 0x1f;
	memcpy(filename.text, ptr + 1, filename.length);

	decode(ptr + 16, 36, (char *)&header);

	for (i = 52; isspace(ptr[i]); ++i);
	ptr += i;
	size -= i;

	crc = crc_update(0, &header, sizeof(header) - 3);

	file_size = *(uint32_t *)&header.file_size & 0xffffff;
	seg_start = *(uint32_t *)&header.seg_start & 0xffffff;
	seg_len = *(uint32_t *)&header.seg_len & 0xffffff;

	if (crc != *(uint16_t *)&header.crc) return BS_BAD_CRC;
	if (seg_start != 0 || seg_len != file_size) return BS_PARTIAL; // partial file

	// check for directory file type / aux type?

	refNum = FilePrompt((GSString255 *)&filename, header.file_type, *(uint16_t *)&header.aux_type);
	if (refNum == 0) return BS_USER_CANCELED;

	ioDCB.refNum = refNum;
	closeDCB.refNum = refNum;


	unsigned offset = 0;
	unsigned j = 0;
	unsigned count = (seg_len + 47) / 48; // assume < 3MB
	crc = 0;
	for(j = 0; j < count; ++j) {

		if (size < 64 + 4) return BS_TRUNCATED;

		if (offset == 1536) {
			crc = crc_update(crc, buffer, 1536);

			ioDCB.requestCount = 1536;
			WriteGS(&ioDCB);

			offset = 0;
			file_size -= 1536;
		}

		decode(ptr, 64, buffer + offset);
		offset += 48;

		for (i = 64; isspace(ptr[i]); ++i);
		ptr += i;
		size -= i;
	}

	if (offset) {
		crc = crc_update(crc, buffer, offset);

		ioDCB.requestCount = file_size;
		WriteGS(&ioDCB);

	}
	CloseGS(&closeDCB);

	// todo - set create/mod dates?

	if (size < 4) return BS_TRUNCATED; // trailing crc.
	decode(ptr, 4, header.crc);
	if (crc != *(uint16_t *)&header.crc) return BS_BAD_CRC;

	return BS_OK;
}

#define MAGIC "FiLeStArTfIlEsTaRt\r"


int DecodeBINSCII(const uint8_t *ptr, long size) {

	/* find the sentinel... */

	unsigned max;
	unsigned i;
	unsigned c;
	unsigned prev = '\r';


	//expect it within the first 32k...
	if (size < 18) return error(BS_NOT_BINSCII);

	if (size >> 16) max = 0x8000;
	else max = size - 18;
	for (i = 0; i < max; ++i) {
		c = ptr[i];
		if (c == 'F' && prev == '\r') {
			if (i + 18 >= size) return error(BS_NOT_BINSCII);

			if (!memcmp(ptr + i, MAGIC, 19)) {
				i += 19;
				size -= i;
				ptr += i;
				return error(de_scii(ptr, size));
			}
		}
		prev = c;
	}
	return error(BS_NOT_BINSCII);
}

#ifdef TEST

const char file[] = 
"FiLeStArTfIlEsTaRt\r"
"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789()\r"
"Ax              AAQAAAAAAAwwIEAA2LFAS5wKOsi9AAQAAgXx\r"
"AAAeAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\r"
"AgN1\r"
;

int main(int argc, char **argv) {

	return DecodeBINSCII(file, sizeof(file));
}

#endif
