/*
 * rfc 4266
 *
 */

#pragma noroot
#pragma optimize 79

#include "gopher.h"

#include <string.h>
#include <ctype.h>
#include <stdlib.h>


typedef struct range {
	unsigned location;
	unsigned length;
} range;

typedef struct url {
	range host;
	range selector;
	unsigned port;
	unsigned type;
} url;


window_cookie *parse_url(const char *cp, unsigned length) {

	// leading gopher:// is optional.
	// any other scheme is an error.
	// unsigned bits = 0;
	unsigned i;

	// expect host [:port] /
	range host = { 0, 0 };
	range selector = { 0, 0 };
	unsigned port = 70;
	unsigned type = '1';
	unsigned st;
	unsigned extra;
	window_cookie *cookie;
	char *p;

	st = 0;
	if (length >= 9 && !memcmp(cp, "gopher://", 9)) {
		cp += 9;
		length -= 9;
	}

	for (i = 0; i < length; ++i) {
		unsigned c = cp[i];
		switch(st) {
			case 0:
				if (c == '/') {
					host.length = i - host.location;
					st = 3;
				}
				else if (c == ':') {
					host.length = i - host.location;
					port = 0;
					st = 2;
				}
				break;

			case 1:
				// host
				if (c == '/') {
					host.length = i - host.location;
					st = 3;
				}
				else if (c == ':') {
					host.length = i - host.location;
					port = 0;
					st = 2;
				}
				break;

			case 2:
				// port
				if (isdigit(c)) {
					port = port * 10;
					port += (c & 0x0f);
				}
				else if (c == '/') {
					// catches http://
					if (port == 0) return NULL;
					st = 3;
				} else return NULL;
				break;

			case 3:
				// type
				type = c;
				selector.location = i + 1;
				++st;
				break;
			case 4:
				// selector
				break;
		}
	}

	if (st <= 1) {
		host.length = length - host.location;
	}
	if (st == 4) {
		selector.length = length - selector.location;
	}


	extra = 3 + 3 + 2 + (host.length << 1) + (selector.length << 1);

	cookie = malloc(sizeof(window_cookie) + extra);
	if (!cookie) return NULL;

	memset(cookie, 0, sizeof(window_cookie) + extra);

	cookie->type = type;
	cookie->port = port;

	p = cookie->data;
	cookie->title = p;
	if (selector.length) {
		// host + selector
		*p++ = host.length + selector.length + 3 + 2;
		*p++ = ' ';
		memcpy(p, cp + host.location, host.length);
		p += host.length;
		*p++ = ' ';
		*p++ = '-';
		*p++ = ' ';
		memcpy(p, cp + selector.location, selector.length);
		p += selector.length;
		*p++ = ' ';
	} else {
		// just the host
		*p++ = host.length + 2;
		*p++ = ' ';
		memcpy(p, cp + host.location, host.length);
		p += host.length;
		*p++ = ' ';
	}
	cookie->host = p;
	*p++ = host.length;
	memcpy(p, cp + host.location, host.length);
	p += host.length;

	if (selector.length) {
		cookie->selector = p;
		*p++ = selector.length;
		memcpy(p, cp + selector.location, selector.length);
		p += selector.length;
	}
	return cookie;
}
