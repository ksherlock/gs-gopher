
#include <tcpip.h>

typedef struct window_cookie {
	dnrBuffer dnr;
	unsigned port;
	unsigned type;
	unsigned ipid;
	char *title; // window title
	char *host;
	char *selector;

	char data[1]; // host/selector/title pstrings.

} window_cookie;

enum {
	kGopherTypeText = '0', 
	kGopherTypeIndex = '1',
	kGopherTypeInfo = 'i',
	kGopherTypeError = '3',
	kGopherTypeSearch = '7',
};

window_cookie *parse_url(const char *cp, unsigned length);
