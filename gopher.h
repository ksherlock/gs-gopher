
#include <stdint.h>
#include <types.h>

/*
 * The UMN gopherd (version 3) considers these to be text files:
 * '0' (text), '1' (index), 'M' (MIME), '2' (CSO), '4' (Mac Hex)
 * 'h' (HTML) was not, for reasons.
 * '6' (uuencoded file) is also not.
 *
 * Additionally, these mime types are text:
 * message/rfc822 (email)
 * application/postscript
 * application/mac-binhex40
 * application/rtf (often 'r')
 * application/gopher*
 *
 * Most of those should probably just be saved. 
 */


enum {
	kGopherTypeText = '0', 
	kGopherTypeIndex = '1',
	kGopherTypeInfo = 'i',
	kGopherTypeError = '3',
	kGopherTypeSearch = '7',
};


// records for the List Manager index.
typedef struct ListEntry {
	// ptrs are p-strings.
	char *name;
	uint8_t flags;
	uint8_t type;
	uint16_t port;
	char *selector;
	char *host;
} ListEntry;




// window refcon
typedef struct cookie {
	unsigned type;
	unsigned menuID;
	char *title; // window title
} cookie;

typedef struct text_cookie {
	unsigned type;
	unsigned menuID;
	char *title;
	char data[1];
} text_cookie;

typedef struct index_cookie {
	unsigned type;
	unsigned menuID;
	char *title;
	Handle handle;
	unsigned listSize;
	ListEntry *list;
	char data[1];
} index_cookie;



void NewTextWindow(text_cookie *, void *, unsigned long);
void NewIndexWindow(index_cookie *);

int DecodeBINSCII(const unsigned char *ptr, long size);

