
#include <tcpip.h>

typedef struct window_cookie {
	dnrBuffer dnr;
	unsigned port;
	unsigned type;
	char *host;
	char *selector;
	char *title; // window title

	char data[1]; // host/selector/title pstrings.

} window_cookie;

enum {
	kGopherTypeText = '0', 
	kGopherTypeIndex = '1',
	kGopherTypeInfo = 'i',
	kGopherTypeError = '3',
	kGopherTypeSearch = '7',
};

window_cookie *parse_url(char *str, unsigned length);
