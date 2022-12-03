#pragma lint -1
#pragma optimize 79

#include <control.h>
#include <desk.h>
#include <event.h>
#include <gsos.h>
#include <lineedit.h>
#include <locator.h>
#include <memory.h>
#include <menu.h>
#include <misctool.h>
#include <qdaux.h>
#include <quickdraw.h>
#include <resources.h>
#include <tcpip.h>
#include <window.h>


#include <stdlib.h>
#include <string.h>

#include "defines.h"
// #include "url.h"
#include "gopher.h"



unsigned MyID;

static Pointer Icons[4];


void TCPNewWindow(GrafPortPtr window, window_cookie *cookie) {

	if (TCPIPValidateIPString(cookie->host)) {
		cvtRec cvt;
		unsigned err;

		TCPIPConvertIPToHex(&cvt, cookie->host);
		cookie->dnr.DNRstatus = DNR_OK;
		cookie->dnr.DNRIPaddress = cvt.cvtIPAddress;
		cookie->state = kStateDNR; // handle later
#if 0
		cookie->state = kStateConnect;
		cookie->ipid = TCPIPLogin(MyID, cvt.cvtIPAddress, cookie->port, 0, 64);
		if (_toolErr) {
			cookie->state = kStateError;
		}
		err = TCPIPOpenTCP(cookie->ipid);
#endif
	} else {
		TCPIPDNRNameToIP(cookie->host, &cookie->dnr);
		cookie->state = kStateDNR;
	}
}

void TCPCloseWindow(GrafPortPtr window, window_cookie *cookie) {

	switch(cookie->state) {
	case kStateError:
	case kStateComplete:
		break;
	case kStateDNR:
		TCPIPCancelDNR(&cookie->dnr);
		break;
	case kStateConnect:
	case kStateRead:
	case kStateClosing:
		TCPIPAbortTCP(cookie->ipid);
		TCPIPLogout(cookie->ipid);
		cookie->ipid = 0;
		break;
	}
}

void TCPProcessWindow(GrafPortPtr window, window_cookie *cookie) {

	unsigned err;
	srBuff sr;
	unsigned ipid = cookie->ipid;

	switch (cookie->state) {
		case kStateError:
		case kStateComplete:
			break;

		case kStateDNR:
			if (cookie->dnr.DNRstatus == DNR_OK) {
				cookie->state = kStateConnect;

				ipid = TCPIPLogin(MyID, cookie->dnr.DNRIPaddress, cookie->port, 0, 64);
				if (_toolErr) {
					cookie->state = kStateError;
					return;
				}
				err = TCPIPOpenTCP(ipid);
				if (_toolErr || err) {
					TCPIPLogout(ipid);
					cookie->state = kStateError;
					return;
				}
				cookie->ipid = ipid;
				break;
			}
			if (cookie->dnr.DNRstatus == DNR_Pending) return;
			// timeout / no dns
			cookie->state = kStateError;
			break;

		case kStateConnect:
			err = TCPIPStatusTCP(ipid, &sr);
			if (sr.srState == TCPSESTABLISHED) {
				unsigned char *selector = cookie->selector;
				cookie->state = kStateRead;
				if (selector)
					err = TCPIPWriteTCP(ipid, selector+1, selector[0], 0, 0);
				err = TCPIPWriteTCP(ipid, "\r\n", 2, 1, 0);
				cookie->state = kStateRead;
			}
			else if (sr.srState == TCPSCLOSED) {
				// reset??
				TCPIPLogout(ipid);
				cookie->ipid = 0;
				cookie->state = kStateError;
			}
			else if (sr.srState > TCPSESTABLISHED) {
				// closing
				cookie->state = kStateError;
			}
			break;

		case kStateRead:
			err = TCPIPStatusTCP(ipid, &sr);
			if (sr.srRcvQueued) {

			}

			if (sr.srState == TCPSCLOSED) {
				// reset??
				TCPIPLogout(ipid);
				cookie->ipid = 0;
				cookie->state = kStateError;
			}
			else if (sr.srState > TCPSESTABLISHED) {
				// closing...
				TCPIPCloseTCP(ipid);
				cookie->state = kStateClosing;
			}
			break;
		case kStateClosing:
			err = TCPIPStatusTCP(ipid, &sr);
			if (sr.srState == TCPSTIMEWAIT) {
				TCPIPAbortTCP(ipid);
				TCPIPLogout(ipid);
				cookie->ipid = 0;
				cookie->state = kStateComplete;
			}
			if (sr.srState == TCPSCLOSED) {
				TCPIPLogout(ipid);
				cookie->ipid = 0;
				cookie->state = kStateComplete;				
			}
			break;
	}
}

static void Setup(void) {

	/* menu bars */

	SetSysBar(NewMenuBar2(2, kMenuBarID, 0));
	SetMenuBar(0);
	FixAppleMenu(kAppleMID);

	FixMenuBar();
	DrawMenuBar();

	InitCursor();

	/* icons */
	Icons[0] = (Pointer)GetSysIcon(getFileIcon, 0x04, 0x0000); // text
	Icons[1] = (Pointer)GetSysIcon(getFileIcon, 0x0f, 0x0000); // folder
	Icons[2] = (Pointer)GetSysIcon(getFileIcon, 0x06, 0x0000); // binary.
	Icons[3] = (Pointer)GetSysIcon(getFileIcon, 0xdd, 0x0000); // document
}

Pointer IconForType(unsigned type) {
	switch(type) {
		case '0': return Icons[0]; /* text */
		case '1': return Icons[1]; /* directory/index */
		case '9': return Icons[2]; /* binary */
		default: return Icons[3];
	}
}

static WmTaskRec event;
unsigned Quit = 0;



enum {
	kOpenReturn = 1,
	kOpenEscape,
	kOpenAppleA,
	kOpenControlA,
	kOpenControlE
};

#pragma toolparms 1
#pragma databank 1
#define CTRL(x) (x & 0x1f)
void OpenEventHook(WmTaskRec *event) {
	// Line Edit is pretty dumb about control characters.
	// this filters out control characters (and converts end-of-edit chars to app4 events)

	unsigned what = event->what;
	if (what == keyDownEvt || what == autoKeyEvt) {
		unsigned key = event->message;

		if (key < 0x20) {

			switch(key) {
				case CTRL('A'):
					event->what = app4Evt;
					event->message = kOpenControlA;
					break;

				case CTRL('E'):
					event->what = app4Evt;
					event->message = kOpenControlE;
					break;

				case CTRL('M'):
					event->what = app4Evt;
					event->message = kOpenReturn;
					break;

				case CTRL('['):
					event->what = app4Evt;
					event->message = kOpenEscape;
					break;
				case 0x08:
				case 0x15:
					// left/right arrow keys
					break;
				default:
				event->what = nullEvt;
				event->message = 0;
			}
		}
		else if (event->modifiers & appleKey) {

			if (key == '.') {
				event->what = app4Evt;
				event->message = kOpenEscape;
			}
			else if (key == 'a' || key == 'A') {
				event->what = app4Evt;
				event->message = kOpenAppleA;
			}

		}
		else if ((event->modifiers & keyPad) && key >= 0x60) {
			// extended key
			event->what = nullEvt;
			event->message = 0;
		}
	}
}
#pragma toolparms 0
#pragma databank 0

void DoOpen(void) {

	static char text[256];
	// static URLComponents uc;

	window_cookie *cookie = NULL;

	CtlRecHndl ctrlH;
	LERecHndl leH;

	GrafPortPtr win = NewWindow2(NULL, NULL, NULL, NULL, refIsResource, kURLWindow, rWindParam1);

	ctrlH = GetCtlHandleFromID(win, kGopherURL);
	leH = (LERecHndl)GetCtlTitle(ctrlH);

	// ShowWindow(URLWindow);
	// event.wmTaskMask = 0x001F3007;

	for(;;) {
		#define flags mwUpdateAll | mwDeskAcc | mwIBeam
		// DoModalWindow(&event, NULL, (VoidProcPtr)0x80000000, (VoidProcPtr)-1, flags);
		DoModalWindow(&event, NULL, (VoidProcPtr)OpenEventHook, (VoidProcPtr)-1, flags);
		#undef flags
		// break on return, esc (apple-. converted to esc)

		if (event.what == app4Evt) {

			unsigned quit = 0;
			switch((unsigned)event.message) {
				case kOpenAppleA:
					// select-all
					LESetSelect(0, 256, leH);
					break;
				case kOpenControlA:
					LESetSelect(0, 0, leH);
					break;
				case kOpenControlE:
					LESetSelect(256, 256, leH);
					break;
				case kOpenEscape:
					quit = 1;
					break;
				case kOpenReturn:
					GetLETextByID(win, kGopherURL, (StringPtr)text);
					if (!text[0]) break;

					cookie = parse_url(text + 1, text[0]);
					if (cookie) {
						quit = 1;
					} else {
						SysBeep2(sbBadInputValue);
					}
					break;

			}
			if (quit) break;
		}

	}
	CloseWindow(win); // or just hide so url is retained?
	InitCursor(); /* reset possible I-beam cursor */

	if (cookie) {
		unsigned long id = 0;
		if (cookie->type == kGopherTypeText) id = kTextWindow;
		if (cookie->type == kGopherTypeIndex) id = kIndexWindow;
		if (id) {
			GrafPortPtr win;
			win = NewWindow2((Pointer)cookie->title, (Long)cookie, NULL, NULL, refIsResource, id, rWindParam1);
			SetInfoRefCon((Long)cookie, win);

		} else {
			free(cookie);
		}
	}

	// event.wmTaskMask = 0x001FFFFF;
}



void DoMenu(void) {
	unsigned item = event.wmTaskData & 0xffff;
	unsigned menu = event.wmTaskData >> 16;

	HiliteMenu(0, menu);
	switch (item) {
		case kUndoItem:
		case kCutItem:
		case kCopyItem:
		case kPasteItem:
		case kClearItem:
		case kCloseItem:
			break;
		case kAbout:
			AlertWindow(refIsResource << 1, NULL, 1);
			break;
		case kOpenItem:
			DoOpen();
			break;
		case kQuitItem:
			Quit = 1;
			break;
	}
}

void EventLoop(void) {
	word taskCode;

	event.wmTaskMask = 0x001FFFFF;
	while (!Quit) {
		taskCode = TaskMaster(0xffff, &event);
		switch (taskCode) {
		case wInSpecial:
		case wInMenuBar:
			DoMenu();
			break;
		case wInControl:
			break;

		}
	}
}

GrafPortPtr Window;

#pragma databank 1
#pragma toolparms 1

void DrawWindow(void) {
	DrawControls(Window);
}

#pragma databank 0
#pragma toolparms 0

extern void *IndexControl;
void CreateWindow(void) {
	Window = NewWindow2(NULL, NULL, DrawWindow, NULL, refIsResource, kIndexWindow, rWindParam1);
	SetPort(Window);

	IndexControl = GetCtlHandleFromID(Window, kGopherIndex);
}

int main(void) {

	Ref tlRef;
	static QuitRecGS quitDCB = { 2, 0, 0 };

	TLStartUp();
	MyID = MMStartUp();

	tlRef = StartUpTools(MyID, refIsResource, (Ref)kStartStopID);


	Setup();
	// CreateWindow();
	EventLoop();

	// CloseWindow(Window);

	ShutDownTools(refIsHandle, tlRef);
	MMShutDown(MyID);
	TLShutDown();
	QuitGS(&quitDCB);
}
