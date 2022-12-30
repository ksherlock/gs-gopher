#pragma lint -1
#pragma optimize 79

#include <control.h>
#include <desk.h>
#include <event.h>
#include <gsos.h>
#include <lineedit.h>
#include <list.h>
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
#include "gopher.h"
#include "q.h"


const char *ReqName = "\pTCP/IP~kelvin~gopher~";

unsigned MyID;

static Pointer Icons[4];


unsigned window_count = 0;
unsigned window_active = 0;
GrafPortPtr windows[10];


#pragma toolparms 1
#pragma databank 1

pascal void WindowDrawControls(void) {
	DrawControls(GetPort());
}

#pragma toolparms 0
#pragma databank 0

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

static Pointer GetIcon(unsigned ix) {
	Handle h;

	h = LoadResource(rIcon, ix);
	if (h) {
		HLock(h);
		DetachResource(rIcon, ix);
		return *(void **)h;
	}
	return 0;
}

static void Setup(void) {

	Handle h;
	/* menu bars */

	SetSysBar(NewMenuBar2(2, kMenuBarID, 0));
	SetMenuBar(0);
	FixAppleMenu(kAppleMID);

	FixMenuBar();
	DrawMenuBar();

	InitCursor();
	StartupQueue();

#if 0
	/* icons */
	Icons[0] = (Pointer)GetSysIcon(getFileIcon, 0x04, 0x0000); // text
	Icons[1] = (Pointer)GetSysIcon(getFileIcon, 0x0f, 0x0000); // folder
	Icons[2] = (Pointer)GetSysIcon(getFileIcon, 0x06, 0x0000); // binary.
	Icons[3] = (Pointer)GetSysIcon(getFileIcon, 0xdd, 0x0000); // document
#endif

	Icons[0] = (Pointer)GetIcon(kIconText); // text
	Icons[1] = (Pointer)GetIcon(kIconFolder); // folder
	Icons[2] = (Pointer)GetIcon(kIconBinary); // binary.
	Icons[3] = (Pointer)GetIcon(kIconDocument); // document


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


void DoAbout(void) {

#if 0
	AlertWindow(refIsResource << 1, NULL, 1);
#else
	GrafPortPtr win;
	long id;

	win = NewWindow2(NULL, NULL, WindowDrawControls, NULL, refIsResource, kAboutWindow, rWindParam1);

	for(;;) {
		#define flags mwUpdateAll | mwDeskAcc | mwIBeam
		id = DoModalWindow(&event, NULL, NULL, (VoidProcPtr)-1, flags);
		#undef flags

		if (id == kAboutButton) break;

		ProcessQueue();
	}

	CloseWindow(win);
	InitCursor();
#endif
}


void DoOpen(void) {

	static char text[256];
	// static URLComponents uc;

	CtlRecHndl ctrlH;
	LERecHndl leH;
	unsigned ok;


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

					ok = QueueURL(text + 1, text[0]);
					if (ok) {
						quit = 1;
						break;
					}
					SysBeep2(sbBadInputValue);
					break;

			}
			if (quit) break;
		}

		if (event.what == nullEvt) {
			ProcessQueue();
		}
	}
	CloseWindow(win); // or just hide so url is retained?
	InitCursor(); /* reset possible I-beam cursor */
}


void NewTextWindow(text_cookie *cookie, void *text, unsigned long length) {

	GrafPortPtr win;
	Handle teH;

	win = NewWindow2(cookie->title, (Long)cookie, WindowDrawControls, NULL, refIsResource, kTextWindow, rWindParam1);
	if (_toolErr) {
		return;
	}

	teH = (Handle)GetCtlHandleFromID(win, kGopherText);

	TESetText(teDataIsTextBlock | teTextIsPtr, (Ref)text, length, 0, NULL, teH);
}
#pragma toolparms 1
#pragma databank 1
void pascal ListDraw(Rect *rectPtr, ListEntry *entry, Handle listHandle) {

	static unsigned char DimMask[] = {
		0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa,
	};

	static unsigned char NorMask[] = {
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	};

	void *iconPtr = nil;
	unsigned width = rectPtr->h2 - rectPtr->h1 - 32;

	// draw type icon and name.
	unsigned memFlags = entry->flags;



	// fill rect (white or green)
	if (memFlags & memSelected) {
		SetBackColor(0xaaaa); // green
		SetSolidBackPat(0xaaaa);
	}
	else {
		SetBackColor(0xffff); // white
		SetSolidBackPat(0xffff);
	}
	EraseRect(rectPtr);

	iconPtr = IconForType(entry->type);
	if (iconPtr)
		DrawIcon(iconPtr, 0, rectPtr->h1 + 2, rectPtr->v1 + 2);

	SetForeColor(0x0000);
	SetTextMode(0);
	MoveTo(rectPtr->h1 + 32, rectPtr->v2 - 1);

#if 0
	if (memFlags & memDisabled) {
		SetPenMask(DimMask);
	}
#endif

	DrawStringWidth(dswTruncRight + dswPString + dswStrIsPtr, (Ref)entry->name, width);

	if (memFlags & memDisabled) {
		SetPenMask(DimMask);
		EraseRect(rectPtr);
		SetPenMask(NorMask);
	}



}

#pragma toolparms 0
#pragma databank 0

void NewIndexWindow(index_cookie *cookie) {

	GrafPortPtr win;
	CtlRecHndl ctrl;
	ListCtlRec *listPtr;

	win = NewWindow2(cookie->title, (Long)cookie, WindowDrawControls, NULL, refIsResource, kIndexWindow, rWindParam1);
	if (_toolErr) {
		return;
	}

	ctrl = GetCtlHandleFromID(win, kGopherIndex);

	listPtr = *(ListCtlRec **)ctrl;
	listPtr->ctlMemSize = sizeof(ListEntry);

#if 0
	// number of entries / view size.
	// view size 0 = list manager takes care of it.
	listPtr->ctlData = ((unsigned long)cookie->listSize << 16);
	listPtr->ctlMemDraw = ListDraw;
	listPtr->ctlList = (MemRecPtr)cookie->list;
#endif

	NewList2((Pointer)ListDraw, 1, (Ref)cookie->list, refIsPointer, cookie->listSize, (Handle)ctrl);

	ShowWindow(win);
	BringToFront(win);
}

void DoCloseWindow(GrafPortPtr win) {
	struct cookie *c;

	if (!win) win = FrontWindow();
	if (!win) return;

	c = (struct cookie *)GetWRefCon(win);
	if (c) {
		if (c->type == kGopherIndex) {
			DisposeHandle(((index_cookie *)c)->handle);
		}
		free(c);
	}

	CloseWindow(win);

	// todo -- update menus, etc.

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
			break;
		case kCloseItem:
			DoCloseWindow(0);
			break;

		case kAbout:
			DoAbout();
			break;
		case kOpenItem:
			DoOpen();
			break;
		case kQuitItem:
			Quit = 1;
			break;

		case kConnectItem:
			TCPIPConnect(NULL);
			break;

		case kDisconnectItem:
			TCPIPDisconnect(1, NULL);
			break;
	}
}

void OpenIndex(ListCtlRec *rec) {
	ListEntry *e = (ListEntry *)rec->ctlList;
	unsigned count = rec->ctlData >> 16;

	unsigned i;

	for (i = 0; i < count; ++i, ++e) {
		if (e->flags & memSelected) {
			QueueEntry(e);
			break;
		}
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

		case wInGoAway: {
			// wmTaskData = window
			GrafPortPtr win = (GrafPortPtr)event.wmTaskData;
			DoCloseWindow(win);
			break;
		}

		case wInControl: {
			// wmTaskData = window
			// wmTaskData2 = control (handle)
			// wmTaskData3 = def proc result code
			// wmTaskData4 = control id
			if (event.wmClickCount == 2 && event.wmTaskData4 == kGopherIndex) {
				ListCtlRec **handle = (ListCtlRec **)event.wmTaskData2;
				OpenIndex(*handle);
			}

			break;
		}

		case autoKeyEvt:
		case keyDownEvt: {
			// keyboard navigation for index?

			GrafPortPtr win = (GrafPortPtr)FrontWindow();
			if (GetSysWFlag(win)) break; // NDA, etc.


			CtlRecPtr ctrlPtr;

			CtlRecHndl ctrlH = GetCtlHandleFromID(win, kGopherIndex); // FindTargetCtl();
			if (_toolErr || !ctrlH) break;

			ctrlPtr = *ctrlH;
			if (ctrlPtr->ctlID == kGopherIndex) {
				// pass up/down events to 
				switch(event.message) {
				case 0x0a:
				case 0x0b:
					// up/down
					SetPort(win);
					ListKey(0, &event, ctrlH);
					break;
				case ' ':
				case 0x0d:
					OpenIndex((ListCtlRec *)ctrlPtr);
					break;
				}

			}

#if 0
			GrafPortPtr win = (GrafPortPtr)FrontWindow();
			if (GetSysWFlag(win)) break; // NDA, etc.
			asm {
				ldx #event
				ldy #^event
				brk 0xea
			}
#endif

			break;
		}

		case mouseDownEvt:
			break;

		case nullEvt:
			ProcessQueue();
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
