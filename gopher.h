
#include <stdint.h>
#include <types.h>

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
	char *title; // window title
} cookie;

typedef struct text_cookie {
	unsigned type;
	char *title;
	char data[1];
} text_cookie;

typedef struct index_cookie {
	unsigned type;
	char *title;
	Handle handle;
	unsigned listSize;
	ListEntry *list;
	char data[1];
} index_cookie;



void NewTextWindow(text_cookie *, void *, unsigned long);
void NewIndexWindow(index_cookie *);
