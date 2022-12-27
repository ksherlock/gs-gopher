#pragma lint -1
#pragma optimize 79
#pragma noroot


#include <memory.h>
#include <tcpip.h>

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "q.h"
#include "gopher.h"

enum {
	kStateComplete = 0,
	kStateDNR,
	kStateConnecting,
	kStateReading,
	kStateClosing,
	kStateError
};


typedef struct DownloadItem {

	unsigned state;
	unsigned ipid;
	unsigned type;
	unsigned port;

	unsigned error; /* tool error, tcp err if < 0x100) */

	dnrBuffer dnr;
	char *host;
	char *selector;
	unsigned long tick;


	// download data
	Handle handle;
	unsigned long size;
	unsigned lines;

} DownloadItem;

/* extra errors */
enum {
	terrDNR_Failed = 0x50 + DNR_Failed,
	terrDN_NoDNSEntry = 0x50 + DNR_NoDNSEntry
};

DownloadItem DownloadQueue[16];

unsigned Active = 0;
unsigned ID = 0;

void CleanupItem(DownloadItem *item, unsigned new_state, unsigned new_error);

void DownloadComplete(DownloadItem *item);

void StartupQueue(void) {
	ID = MMStartUp() | 0x0f00;
	Active = 0;
	memset(DownloadQueue, 0, sizeof(DownloadQueue));
}

void ShutDownQueue(void) {
	CancelQueue();
}


void CancelQueue(void) {
	/* application is shutting down or whatever. cancel everything */
	unsigned mask;
	DownloadItem *item = DownloadQueue;

	if (!Active) return;

	for (mask = 1; mask; mask <<= 1, ++item) {

		if (!(Active & mask)) continue;

		switch(item->state) {
		case kStateDNR:
			TCPIPCancelDNR(&item->dnr);
			break;
		case kStateConnecting:
		case kStateReading:
		case kStateClosing:
			if (item->ipid) {
				// TCPIPCloseTCP(item->ipid);
				TCPIPAbortTCP(item->ipid);
				TCPIPLogout(item->ipid);
			}
			break;
		}
		if (item->handle) DisposeHandle(item->handle);
		if (item->host) free(item->host);
	}
	memset(DownloadQueue, 0, sizeof(DownloadQueue));
	Active = 0;
}

void CleanupItem(DownloadItem *item, unsigned new_state, unsigned new_error) {

	switch(item->state) {
		case kStateDNR:
			TCPIPCancelDNR(&item->dnr);
			break;
		case kStateConnecting:
		case kStateReading:
		case kStateClosing:
			if (item->ipid) {
				// TCPIPCloseTCP(item->ipid);
				TCPIPAbortTCP(item->ipid);
				TCPIPLogout(item->ipid);
			}
			break;
	}
	if (item->handle) DisposeHandle(item->handle);
	if (item->host) free(item->host);
	memset(item, 0, sizeof(*item));
	item->state = new_state;
	item->error = new_error;
}


// trailing . not handled here.
static unsigned ReadText(DownloadItem *item) {

	static char buffer[1024];
	static rrBuff rr;
	static srBuff sr;


	char *cp;
	unsigned long capacity;
	unsigned long needed;

	unsigned ipid = item->ipid;
	Handle h = item->handle;

	if (h) {
		capacity = GetHandleSize(h);
		cp = *(char **)h;
		cp += item->size;
	} else {
		h = NewHandle(0x4000, ID, attrLocked | attrNoSpec, 0);
		if (_toolErr) {
			CleanupItem(item, kStateError, _toolErr);
			return 0;
		}
		capacity = 0x4000;
		cp = *(char **)h;
		item->size = 0;
		item->handle = h;
	}


	/*
	 * When reading into a buffer, TCPIPReadTCP will not (currently)
	 * do a short read. that is, if there is 25 bytes of data available 
	 * and you try to read 26 bytes of data, it will read 0 bytes of data.
	 * The exception is if the connection is closing, in which case it
	 * will read 25 bytes of data.
	 */

	for(;;) {
		unsigned i, j;

		TCPIPPoll();
		TCPIPStatusTCP(ipid, &sr);

		i = sizeof(buffer);
		if (sr.srRcvQueued == 0) break;
		if (sr.srRcvQueued < i) i = sr.srRcvQueued;

		TCPIPReadTCP(ipid, 0, (Ref)buffer, i, &rr);

		if (!rr.rrBuffCount) break;

		// reserve/allocate handle space
		needed = item->size + rr.rrBuffCount;
		if (needed > capacity) {
			capacity += needed;
			capacity += 0x3fff; // 16k
			capacity &= ~0x3fffL;
			HUnlock(h);
			SetHandleSize(capacity, h);
			if (_toolErr) {
				CleanupItem(item, kStateError, _toolErr);
				return 0;
			}
			HLock(h);
			cp = *(char **)h;
			cp += item->size;
		}


		for (i = 0, j = 0; i < (unsigned)rr.rrBuffCount; ++i) {
			unsigned c = buffer[i];
			if (c == '\r') { continue; }
			if (c == '\n') {
				c = '\r';
				item->lines++;
			}
			cp[j++] = c;
		}
		item->size += j;
		cp += j;

		if (!rr.rrMoreFlag) break;
	}
	return 1;
}


/* returns 0 if error/complete */
static unsigned OneItem(DownloadItem *item) {

	unsigned err;
	static srBuff sr;
	unsigned ipid;
	unsigned ok;


	ipid = item->ipid;
	if (ipid) {
		err = TCPIPStatusTCP(ipid, &sr);
	}


	switch(item->state) {

		case kStateDNR:
			if (item->dnr.DNRstatus == DNR_OK) {

				item->state = kStateConnecting;

				ipid = TCPIPLogin(ID, item->dnr.DNRIPaddress, item->port, 0, 64);
				if (_toolErr) {
					CleanupItem(item, kStateError, _toolErr);
					return 0;
				}
				err = TCPIPOpenTCP(ipid);
				if (_toolErr || err) {
					TCPIPLogout(ipid);
					CleanupItem(item, kStateError, _toolErr);
					return 0;
				}
				item->ipid = ipid;
				break;
			}
			if (item->dnr.DNRstatus == DNR_Pending) return 1;

			// timeout / no dns
			CleanupItem(item, kStateError, 0x50 + item->dnr.DNRstatus);
			return 0;
			break;

		case kStateConnecting:
			if (sr.srState == TCPSESTABLISHED) {
				unsigned char *selector = item->selector;
				item->state = kStateReading;
				if (selector)
					err = TCPIPWriteTCP(ipid, selector+1, selector[0], 0, 0);
				err = TCPIPWriteTCP(ipid, "\r\n", 2, 1, 0);
				item->state = kStateReading;
			}
			else if (sr.srState == TCPSCLOSED) {
				// reset??
				CleanupItem(item, kStateError, err);
				return 0;
			}
			else if (sr.srState > TCPSESTABLISHED) {
				// closing??
				CleanupItem(item, kStateError, err);
				return 0;
			}
			break;

		case kStateReading:
			if (sr.srRcvQueued) {
				ok = ReadText(item);
				if (!ok) return 0;
			}

			if (sr.srState == TCPSCLOSED) {
				// reset??
				// download complete?
				CleanupItem(item, kStateError, err);
				return 0;
			}
			else if (sr.srState > TCPSESTABLISHED) {
				// closing...
				// open window...
				TCPIPCloseTCP(ipid);
				item->state = kStateClosing;
				DownloadComplete(item);
				DisposeHandle(item->handle);
				item->handle = 0;
			}
			break;


		case kStateClosing:
			if (sr.srState == TCPSTIMEWAIT || sr.srState == TCPSCLOSED) {
				CleanupItem(item, kStateComplete, 0);
				return 0;
			}
			break;
	}

	return 1;
}

void ProcessQueue(void) {

	unsigned mask;


	DownloadItem *item = DownloadQueue;

	TCPIPPoll();
	if (!Active) return;

	for (mask = 1; mask; mask <<= 1, ++item) {

		if (!(Active & mask)) continue;

		OneItem(item);
		if (item->state == kStateError) {
			Active &= ~mask;

		}
		if (item->state == kStateComplete) {
			Active &= ~mask;

		}
	}
}


typedef struct range {
	unsigned location;
	unsigned length;
} range;

unsigned QueueURL(const char *cp, unsigned length) {

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
	char *p;


	unsigned mask;
	DownloadItem *item = DownloadQueue;

	// find an empty entry.
	if (Active == -1) return 0;

	for (mask = 1 ; mask; mask <<= 1, ++item) {
		if (!(Active & mask)) break;
	}
	memset(item, 0, sizeof(*item));



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
					if (port == 0) return 0;
					st = 3;
				} else return 0;
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

	item->type = type;
	item->port = port;


	extra = host.length + selector.length + 2;

	p = malloc(extra);
	if (!p) return 0;

	item->host = p;

	*p++ = host.length;
	memcpy(p, cp + host.location, host.length);
	p += host.length;

	if (selector.length) {
		item->selector = p;
		*p++ = selector.length;
		memcpy(p, cp + selector.location, selector.length);
	} else {
		item->selector = 0;
	}

	// initiate DNS lookup....
	if (TCPIPValidateIPString(item->host)) {
		cvtRec cvt;
		unsigned err;

		TCPIPConvertIPToHex(&cvt, item->host);
		item->dnr.DNRstatus = DNR_OK;
		item->dnr.DNRIPaddress = cvt.cvtIPAddress;
		item->state = kStateDNR; // handle later
#if 0
		item->state = kStateConnecting;
		item->ipid = TCPIPLogin(ID, cvt.cvtIPAddress, item->port, 0, 64);
		if (_toolErr) {
			CleanupItem(kStateError, _toolErr);
			return 0;
		}
		err = TCPIPOpenTCP(item->ipid);
#endif
	} else {
		TCPIPDNRNameToIP(item->host, &item->dnr);
		item->state = kStateDNR;
	}
	Active |= mask;
	return 1;
}





unsigned OneLine(char *ptr, ListEntry *e){

	unsigned i = 0;
	unsigned type;
	unsigned c;
	unsigned port;
	unsigned j;

	c = ptr[0];
	if (c == '.') return -1;
	if (c < ' ') return -1;


	e->type = c;


	j = i;
	e->name = ptr + i;
	++i;

	for(;;++i) {
		c = ptr[i];
		if (c < ' ') break;
	}
	ptr[j] = i - j - 1;
	if (c == '\r') return i;

	j = i;
	e->selector = ptr + i;
	++i;

	for(;;++i) {
		c = ptr[i];
		if (c < ' ') break;
	}
	ptr[j] = i - j - 1;
	if (c == '\r') return i;


	j = i;
	e->host = ptr + i;
	++i;

	for(;;++i) {
		c = ptr[i];
		if (c < ' ') break;
	}
	ptr[j] = i - j - 1;
	if (c == '\r') return i;


	port = 0;
	for(;;++i) {
		c = ptr[i];
		if (!isdigit(c)) break;
		port *= 10;
		port += c - '0';
	}
	if (!port) port = 70;
	e->port = port;

	while (ptr[i] != '\r') ++i;
	return i + 1;
}

static unsigned TitleLength(DownloadItem *item) {
	unsigned len = item->host[0] + 3;
	if (item->selector) len += item->selector[0] + 3;
	return len;
}

static char *TitleCopy(char *cp, unsigned len, DownloadItem *item) {
	char *nm;

	*cp++ = len-1;
	*cp++ = ' ';

	nm = item->host;
	len = nm[0];
	memcpy(cp, nm + 1, len);
	cp += len;
	*cp++ = ' ';

	nm = item->selector;
	if (nm) {

		*cp += '-';
		*cp += ' ';
		len = nm[0];
		memcpy(cp, nm + 1, len);
		cp += len;
		*cp++ = ' ';
	}

	return cp;
}


void IndexComplete(DownloadItem *item) {


	Handle h = item->handle;
	long size = item->size;
	char *ptr;
	char *cp;
	index_cookie *cookie;
	unsigned tlen;
	unsigned long extra;

	unsigned lines;
 	ListEntry *e;


	// verify final character is .\r so checks work///

	if (size < 2) {
		// error of some sort...
	}

	// should end w/ .\r.  append trailing \r as a sentinal.


	HUnlock(h);
	SetHandleSize(size + 2, h);
	HLock(h);
 	ptr = *(char **)h;
 	ptr[size] = '\r';
 	ptr[size+1] = '\r';


 	lines = item->lines + 1;
	tlen = TitleLength(item);
	extra = tlen + sizeof(ListEntry) * lines;

	cookie = malloc(sizeof(index_cookie) + extra);
	memset(cookie, 0, sizeof(index_cookie) + extra);

	cookie->type = item->type;

	cp = cookie->data;
	cookie->title = cp;

	cp = TitleCopy(cp, tlen, item);

	e = (ListEntry *)cp;
	cookie->list = e;

 	lines = 0;
	for(;;) {
		unsigned i = OneLine(ptr, e);
		if (i == -1) break;
		ptr += i;
		++lines;
		++e;
	}
	cookie->listSize = lines;
	cookie->handle = h;
	item->handle = 0;

	NewIndexWindow(cookie);
}


void TextComplete(DownloadItem *item) {

	Handle h = item->handle;
	long size = item->size;
	char *ptr = *(char **)h;
	char *cp;
	text_cookie *cookie;
	unsigned i;
	unsigned tlen;

	// check for a trailing .\r
	// may not be present since many gopher servers are not compliant.

	if (size >= 2) {
		if (ptr[size - 2] == '.' && ptr[size - 1] == '\r')
			size -= 2;
	}


	tlen = TitleLength(item);

	cookie = malloc(sizeof(text_cookie) + tlen);

	cp = cookie->data;
	cookie->type = item->type;
	cookie->title = cp;

	TitleCopy(cp, tlen, item);

	NewTextWindow(cookie, ptr, size);
}

void DownloadComplete(DownloadItem *item) {

	switch(item->type) {
	case kGopherTypeIndex:
		IndexComplete(item);
		break;
	case kGopherTypeText:
		TextComplete(item);
		break;
	}
}


