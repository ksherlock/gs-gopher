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
#include <textedit.h>
#include <window.h>


#include <stdlib.h>
#include <string.h>

#include "defines.h"
// #include "url.h"
#include "gopher.h"



const char *ReqName = "\pTCP/IP~kelvin~gopher~";

unsigned MyID;

static Pointer Icons[4];


unsigned window_count = 0;
unsigned window_active = 0;
GrafPortPtr windows[10];
window_cookie *cookies[10];


#pragma databank 1
#pragma toolparms 1
pascal void WindowDrawControls(void) {
	DrawControls(GetPort());
}

pascal void DrawInfo(Rect *rectPtr, window_cookie *cookie, GrafPortPtr w) {

	const char *name = "";
	switch (cookie->state) {
	case kStateError: name ="\pError"; break;
	case kStateDNR: name = "\pDNR"; break;
	case kStateConnecting: name = "\pConnecting"; break;
	case kStateReading: name ="\pReading"; break;
	case kStateClosing: name = "\pClosing"; break;
	case kStateComplete: name ="\pComplete"; break;
	default: name = "\p???"; break;
	}

	MoveTo(rectPtr->h1 + 2, rectPtr->v2 - 2);
	DrawString(name);
}


#pragma databank 0
#pragma toolparms 0

void TCPNewWindow(GrafPortPtr window, window_cookie *cookie) {

	if (TCPIPValidateIPString(cookie->host)) {
		cvtRec cvt;
		unsigned err;

		TCPIPConvertIPToHex(&cvt, cookie->host);
		cookie->dnr.DNRstatus = DNR_OK;
		cookie->dnr.DNRIPaddress = cvt.cvtIPAddress;
		cookie->state = kStateDNR; // handle later
#if 0
		cookie->state = kStateConnecting;
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
	case kStateConnecting:
	case kStateReading:
	case kStateClosing:
		TCPIPAbortTCP(cookie->ipid);
		TCPIPLogout(cookie->ipid);
		cookie->ipid = 0;
		break;
	}
}

// read.
// binary files are as-is
// index/text files should have \r\n after each line,
// end with .\r\n
// (but \n might be missing and )
void TCPRead(GrafPortPtr window, window_cookie *cookie, unsigned long available) {

	static char buffer[1024];
	rrBuff rr;
	Handle teH = NULL;
	unsigned type = cookie->type;
	unsigned ipid = cookie->ipid;
	unsigned err;

	if (type == kGopherTypeText) {
		teH = (Handle)GetCtlHandleFromID(window, kGopherText);

		for(;;) {
			rlrBuff rlr;
			unsigned i, j, c;

			err = TCPIPReadLineTCP(ipid, (char *)(0x80000000|(unsigned long)"\p\n"), 0, (Ref)buffer, sizeof(buffer), &rlr);

			if (!rlr.rlrBuffCount) break;

			// remove \r, convert \n to \r
			for (i = 0, j = 0; i < (unsigned)rlr.rlrBuffCount; ++i) {
				c = buffer[i];
				if (c == '\r') continue;
				if (c == '\n') c = '\r';
				buffer[j++] = c;
			}
			// end of file....
			if (j == 2 && buffer[0] == '.' && buffer[1] == '\r') {
				// should close it...
				TCPIPCloseTCP(ipid);
				cookie->state = kStateClosing;
				j = 0;
			}

			if (j) {
				TERecord **temp = (TERecord **)teH;

  				(**temp).textFlags &= (~fReadOnly);

				TESetSelection((Pointer)-1, (Pointer)-1, teH);
				TEInsert(teDataIsTextBlock | teTextIsPtr, (Ref)buffer, j, 0, NULL, teH);
  				(**temp).textFlags |= fReadOnly;

			}
			if (!rlr.rlrMoreFlag) break;


		}
		return;
	}


#if 0
	for (;;) {

		err = TCPIPReadTCP(ipid, 0, sizeof(buffer), &rr);

		if (rr.rrBuffCount == 0) break;

		if (type == kGopherTypeText) {
			unsigned i, j, c;
			unsigned bits = 0;

			TESetSelection(-1, -1, teH);
			// need to process the data to remove \r and convert \n to \r
			// and strip trailing .\r\n

			for (i = 0, j = 0; i < (unsigned)rr.rrBuffCount; ++i) {
				c = buffer[i];
				if (c == '\r') continue;
				bits <<= 2;

				if (c == '\n') { c = '\r'; bits |= 0b01; }
				if (c == '.') { bits |= 0b10; }
				if ((bits & 0b111111) == 0b011001) {
					j -= 2;
					continue;
				}
				buffer[j++] = c;
			}
			

			TEInsert(teDataIsTextBlock | teTextIsPtr, j, 0, NULL, teH);
		}

		if (!rr.rrMoreFlag) break;
	}
#endif
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
				cookie->state = kStateConnecting;

				ipid = TCPIPLogin(MyID | 0x0f00, cookie->dnr.DNRIPaddress, cookie->port, 0, 64);
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

		case kStateConnecting:
			err = TCPIPStatusTCP(ipid, &sr);
			if (sr.srState == TCPSESTABLISHED) {
				unsigned char *selector = cookie->selector;
				cookie->state = kStateReading;
				if (selector)
					err = TCPIPWriteTCP(ipid, selector+1, selector[0], 0, 0);
				err = TCPIPWriteTCP(ipid, "\r\n", 2, 1, 0);
				cookie->state = kStateReading;
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

		case kStateReading:
			err = TCPIPStatusTCP(ipid, &sr);
			if (sr.srRcvQueued) {

				TCPRead(window, cookie, sr.srRcvQueued);
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


void NetworkUpdate(unsigned up) {
	if (up) {
		EnableMItem(kOpenItem);
		EnableMItem(kDisconnectItem);
		DisableMItem(kConnectItem);
	} else {
		DisableMItem(kOpenItem);
		DisableMItem(kDisconnectItem);
		EnableMItem(kConnectItem);
	}
}

#pragma databank 1
#pragma toolparms 1
pascal word HandleRequest(word request, longword dataIn, longword dataOut) {


	//word app;
	//app = GetCurResourceApp();
	//SetCurResourceApp(MyID);

	if (request == TCPIPSaysNetworkUp) {
		NetworkUpdate(1);
	}
	if (request == TCPIPSaysNetworkDown) {
		NetworkUpdate(0);
	}

	//SetCurResourceApp(app);
	return 0;
}

#pragma databank 0
#pragma toolparms 0


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

	window_active = 0;
	window_count = 0;

	if (TCPIPGetConnectStatus()) {
		NetworkUpdate(1);
	} else {
		NetworkUpdate(0);
	}
	AcceptRequests(ReqName, MyID, &HandleRequest);
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


	if (window_count >= 10) return;

	GrafPortPtr win = NewWindow2(NULL, NULL, WindowDrawControls, NULL, refIsResource, kURLWindow, rWindParam1);

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
		unsigned i;


		unsigned long id = 0;
		if (cookie->type == kGopherTypeText) id = kTextWindow;
		if (cookie->type == kGopherTypeIndex) id = kIndexWindow;
		if (id) {
			GrafPortPtr win;
			win = NewWindow2((Pointer)cookie->title, (Long)cookie, WindowDrawControls, NULL, refIsResource, id, rWindParam1);
			SetInfoRefCon((Long)cookie, win);
			SetInfoDraw(DrawInfo, win);
			TCPNewWindow(win, cookie);
			ShowWindow(win);
			SelectWindow(win);

			for (i = 0; i < 10; ++i) {
				if (!windows[i]) {
					windows[i] = win;
					cookies[i] = cookie;
					++window_count;
					window_active |= (1 << i);
					break;
				}
			}

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

		case nullEvt:
			if (window_active) {
				unsigned bits;
				unsigned i;
				window_cookie *c;

				TCPIPPoll();
				bits = window_active;
				for(i = 0; bits; ++i, bits >>= 1) {
					unsigned st;
					if (!bits & 1) continue;

					c = cookies[i];
					st = c->state;
					TCPProcessWindow(windows[i], c);
					if (st != c->state) {
						st = c->state;
						DrawInfoBar(windows[i]);
						if (st == kStateError || st == kStateComplete) {
							window_active &= ~(1 << i);
						}
					}
				}
			}
			break;

		}
	}
}

#if 0
GrafPortPtr Window;


extern void *IndexControl;
void CreateWindow(void) {
	Window = NewWindow2(NULL, NULL, WindowDrawControls, NULL, refIsResource, kIndexWindow, rWindParam1);
	SetPort(Window);

	IndexControl = GetCtlHandleFromID(Window, kGopherIndex);
}
#endif

int main(void) {

	unsigned tcp = 0;
	Ref tlRef;
	static QuitRecGS quitDCB = { 2, 0, 0 };

	TLStartUp();
	MyID = MMStartUp();

	tlRef = StartUpTools(MyID, refIsResource, (Ref)kStartStopID);


	LoadOneTool(54, 0x0300);
	if (_toolErr) {
		AlertWindow(awCString, NULL,
			(Ref)"24~Marinetti 3.0b11 or newer is required.~^Too Bad");
		goto exit;
	}
	// if (TCPIPVersion() & 0x7fff < 0x0300)
	if (TCPIPLongVersion() < 0x03006011) {
		AlertWindow(awCString, NULL,
			(Ref)"24~Marinetti 3.0b11 or newer is required.~^Too Bad");
		goto exit;
	}
	if (TCPIPStatus() == 0 || _toolErr) {
		TCPIPStartUp();
		tcp = 1;
	}

	Setup();
	// CreateWindow();
	EventLoop();

	// CloseWindow(Window);

	AcceptRequests(ReqName, MyID, NULL);

	if (tcp) {
		TCPIPShutDown();
		UnloadOneTool(54);
	}

exit:
	ShutDownTools(refIsHandle, tlRef);
	MMShutDown(MyID);
	TLShutDown();
	QuitGS(&quitDCB);
}
