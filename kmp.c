#pragma lint -1
#pragma optimize 79
#pragma noroot
#pragma debug 0x8000

#include <memory.h>
// #include <textedit.h>

#include <ctype.h>
#include "gopher.h"


static GSString32 needle;

static char T[32];

void kmp_setup(const char *pstr) {

	unsigned len;
	unsigned cnd;
	unsigned pos;
	unsigned ccnd;


	// check if dirty
	unsigned dirty = 0;
	len = *pstr++;
	if (len != needle.length) dirty = true;
	needle.length = len;
	for (unsigned i = 0; i < len; ++i) {
		unsigned c = pstr[i];
		if (isupper(c)) c |= 0x20;
		if (needle.text[i] != c) dirty = true;
		needle.text[i] = c;
	}
	if (!dirty) return;

	ccnd = needle.text[0];

	T[0] = 0xff;
	for (cnd = 0, pos = 1; pos < len; ++pos, ++cnd) {
		unsigned cpos = needle.text[pos];

		if (cpos == ccnd) {
			T[pos] = T[cnd];
		} else {
			T[pos] = cnd;
			while (/*cnd >= 0*/ cnd != 0xff && cpos != ccnd) {
				cnd = T[cnd];
				ccnd = needle.text[cnd];
			}
		}
	}

	T[pos] = cnd;
}


long kmp_search(const char *text, unsigned long length, unsigned long offset) {
	
	unsigned j;
	unsigned k = 0; // retained between loops.

	if (offset) {
		if (offset > length) return -1;
		text += offset;
		length -= offset;
	}

	// split up into 16-bit chunks so we can use 16-bit indexing.
	while (length) {
		unsigned max = length > 32768 ? 32768 : length;

		for (j = 0 ; j < length ; ) {
			unsigned c = needle.text[k];
			unsigned cc = text[j];

			// if (isupper(c)) c |= 0x20;
			if (isupper(cc)) cc |= 0x20;

			if (c == cc) {
				++j;
				++k;
				if (k == needle.length) {
					// found!
					return j + offset - needle.length;
				}
			} else {
				k = T[k];
				if (k == 0xff) {
					++j;
					k = 0;
				}
			}			
		}

		length -= max;
		offset += max;
		text += max;
	}

	return -1L;
}
