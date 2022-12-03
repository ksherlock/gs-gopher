/*
 * rfc 4266
 *
 */

#pragma noroot
#pragma optimize 79

#include "gopher-url.h"
#include <string.h>
#include <ctype.h>

int parse_gopher_url(const char *cp, unsigned length, url *url) {

	// leading gopher:// is optional.
	// any other scheme is an error.
	// unsigned bits = 0;
	unsigned i;

#if 0
	// search for ://
	for (i = 0; i < length; ++i) {
		bits <<= 2;
		unsigned c = cp[0];
		if (c == ':') {
			bits |= 0b01;
		} else if (c == '/') {
			bits |= 0b11;
			if ((bits & 0b00111111) == 0b00011111) {
				// match!
				i = i - 3;
				if (strncmp(cp, "gopher://", ))
				break;
			}
		}
	}
#endif

	// expect host [:port] /
	range host = {};
	range selector = {};
	unsigned port = 70;
	unsigned st;

	i = 0;
	if (length >= 9 && !memcmp(cp, "gopher://", 9)) {
		// cp += 9;
		// length -= 9;
		i = 9;
	}

	st = 0;
	host.begin = i;
	for (; i < length; ++i) {
		unsigned c = cp[i];
		switch(st) {
			case 0:
				if (c == ':') {
					host.length = i - host.begin;
					port = 0;
					st = 2;
				}
				else if (c == '/') {
					host.length = i - host.begin;
					st = 3;
				}
				break;

			case 1:
				// host
				if (c == '/') {
					host.length = i - host.begin;
					st = 3;
				}
				else if (c == ':') {
					host.length = i - host.begin;
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
					if (port == 0) return 0;
					st = 3;
				} else return 0;
				break;

			case 3:
				// type
				url->type = c;
				selector.begin = i + 1;
				++st;
				break;
			case 4:
				// selector
				break;
		}
	}

	if (st <= 1) {
		host.length = length - host.begin;
		url->type = '1';
	}
	if (st == 4) {
		selector.length = length - selector.begin;
	}
	url->port = port;
	url->host = host;
	url->selector = selector;

	return 1;
}