#pragma lint -1
#pragma optimize 79

#include <control.h>
#include <desk.h>
#include <event.h>
#include <font.h>
#include <gsos.h>
#include <intmath.h>
#include <lineedit.h>
#include <list.h>
#include <locator.h>
#include <memory.h>
#include <menu.h>
#include <misctool.h>
#include <qdaux.h>
#include <quickdraw.h>
#include <resources.h>
#include <scrap.h>
#include <stdfile.h>
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


GrafPortPtr CreateShadowWindow(GrafPortPtr parent) {

	static ParamList p = {
		sizeof(ParamList), // paramLength
		fVis, // frameBits
	};

	RegionHndl rh;

	WindRec *wp = (WindRec *)parent;
	rh = wp->wContRgn;

	p.wPosition = (**rh).rgnBBox; 
	p.wPosition.h1 += 2;
	p.wPosition.h2 += 2;
	p.wPosition.v1 += 1;
	p.wPosition.v2 += 1;

	if (wp->wFrame & fAlert) {
		p.wPosition.h1 -= 10;
		p.wPosition.h2 += 10;
		p.wPosition.v1 -= 4;
		p.wPosition.v2 += 4;
	}

	p.wPlane = parent;
	return NewWindow(&p);
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

// todo -- update for DAs.
// todo -- copy should only be active if there's a text selection or list selection.

void MenuUpdate(int type) {

	switch(type) {
	case 0:
		// front window invalid
		DisableMItem(kSaveItem);
		DisableMItem(kCloseItem);
		DisableMItem(kPageSetupItem);
		DisableMItem(kPrintItem);

		DisableMItem(kUndoItem);
		DisableMItem(kCutItem);
		DisableMItem(kCopyItem);
		DisableMItem(kPasteItem);
		DisableMItem(kClearItem);
		DisableMItem(kSelectAllItem);
		DisableMItem(kWrapTextItem);
		SetMItemMark(0, kWrapTextItem);

		DisableMItem(kAddBookmarkItem);


		break;
	case 1:
		// front window index
		DisableMItem(kSaveItem);
		EnableMItem(kCloseItem);
		DisableMItem(kPageSetupItem);
		DisableMItem(kPrintItem);

		DisableMItem(kUndoItem);
		DisableMItem(kCutItem);
		EnableMItem(kCopyItem);
		DisableMItem(kPasteItem);
		DisableMItem(kClearItem);
		DisableMItem(kSelectAllItem);
		DisableMItem(kWrapTextItem);
		SetMItemMark(0, kWrapTextItem);

		EnableMItem(kAddBookmarkItem);

		break;
	case 2:
		// front window text.
		EnableMItem(kSaveItem);
		EnableMItem(kCloseItem);
		EnableMItem(kPageSetupItem);
		EnableMItem(kPrintItem);

		DisableMItem(kUndoItem);
		DisableMItem(kCutItem);
		EnableMItem(kCopyItem);
		DisableMItem(kPasteItem);
		DisableMItem(kClearItem);
		EnableMItem(kSelectAllItem);
		EnableMItem(kWrapTextItem);
		// todo -- SetMItemMark()

		EnableMItem(kAddBookmarkItem);

		break;
	}
}

void WindowChange(void) {

	static GrafPortPtr PrevWin = (GrafPortPtr)-1;

	struct cookie *c;
	GrafPortPtr win = FrontWindow();
	if (win == PrevWin) return;
	PrevWin = win;

	if (!win) {
		MenuUpdate(0);
	}

	if (GetSysWFlag(win)) {
		// DA window.
		return;
	}

	c = (struct cookie *)GetWRefCon(win);
	if (!c) {
		MenuUpdate(0);
		return;
	}

	if (c->type == kGopherTypeIndex) {
		MenuUpdate(1);
	} else {
		MenuUpdate(2);
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

Handle SystemFont;
Handle FixedFont;

// apple m 08, 8-pt
#define FixedFontID 32732
#define FixedFontSize 8


// should also calculate width of 80-column text.

Handle LoadFixedFont(void) {
	Handle h;

	FontID FF = {
		{ FixedFontID, 0, FixedFontSize}
	};
	FontStatRec fsr;


	FindFontStats(FF, 0, 1, &fsr);
	if (_toolErr || (fsr.resultStats & notFoundBit)) {
		Handle h;
		Handle h2;
		unsigned char *ptr;
		unsigned long size;

		h = LoadResource(rFont, FixedFontID);
		if (_toolErr) return NULL;
		// DetachResource(rFont, FixedFontID);

		HLock(h);
		ptr = *(unsigned char **)h;
		AddFamily(FixedFontID, ptr);
		size = GetHandleSize(h) - 1 - *ptr; // skip pascal name

		h2 = NewHandle(size, MyID, attrNoSpec, 0);
		PtrToHand(ptr + *ptr + 1, h2, size);
		HUnlock(h);
		ReleaseResource(-1, rFont, FixedFontID);
		AddFontVar((FontHndl)h2, 0);
		return h2;
	}

	InstallFont(FF, 0);
	if (_toolErr) return NULL;

	return (Handle)GetFont();
}

static void Setup(void) {

	Handle h;
	/* menu bars */

	FontID FF = {
		{ FixedFontID, 0, FixedFontSize}
	};
	FontStatRec fsr;


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
	Icons[4] = (Pointer)GetIcon(kIconSearch);


	window_active = 0;
	window_count = 0;

	if (TCPIPGetConnectStatus()) {
		NetworkUpdate(1);
	} else {
		NetworkUpdate(0);
	}
	AcceptRequests(ReqName, MyID, &HandleRequest);


	/* Get a handle to 9ptr monaco for fixed-width text */
	SystemFont = (Handle)GetFont();
	FixedFont = LoadFixedFont();
	SetFont((FontHndl)SystemFont);
}

Pointer IconForType(unsigned type) {
	switch(type) {
		case '0': return Icons[0]; /* text */
		case '1': return Icons[1]; /* directory/index */
		case '9': return Icons[2]; /* binary */
		case '7': return Icons[4];
		case 'i': /* informational */
		case '3': /* error */
			return NULL;
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
	GrafPortPtr shadow;
	long id;

	win = NewWindow2(NULL, NULL, WindowDrawControls, NULL, refIsResource, kAboutWindow, rWindParam1);

	shadow = CreateShadowWindow(win);
	for(;;) {
		#define flags mwUpdateAll | mwDeskAcc | mwIBeam
		id = DoModalWindow(&event, NULL, NULL, (VoidProcPtr)-1, flags);
		#undef flags

		if (id == kAboutButton) break;

		ProcessQueue();
	}

	CloseWindow(shadow);
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

	// GrafPortPtr shadow = NewWindow2(NULL, NULL, WindowDrawControls, NULL, refIsResource, kURLWindowShadow, rWindParam1);
	GrafPortPtr win = NewWindow2(NULL, NULL, WindowDrawControls, NULL, refIsResource, kURLWindow, rWindParam1);

	GrafPortPtr shadow = CreateShadowWindow(win);

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
	CloseWindow(shadow);
	CloseWindow(win); // or just hide so url is retained?
	InitCursor(); /* reset possible I-beam cursor */
}


void DoSearch(ListEntry *e) {

	static char text[256];
	// static URLComponents uc;

	CtlRecHndl ctrlH;
	LERecHndl leH;
	unsigned ok;


	if (window_count >= 10) return;

	// GrafPortPtr shadow = NewWindow2(NULL, NULL, WindowDrawControls, NULL, refIsResource, kSearchWindowShadow, rWindParam1);
	GrafPortPtr win = NewWindow2(NULL, NULL, WindowDrawControls, NULL, refIsResource, kSearchWindow, rWindParam1);

	GrafPortPtr shadow = CreateShadowWindow(win);

	ctrlH = GetCtlHandleFromID(win, kSearchLineEdit);
	leH = (LERecHndl)GetCtlTitle(ctrlH);

	// todo -- set the text to the description?

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
					GetLETextByID(win, kSearchLineEdit, (StringPtr)text);
					QueueEntry(e, text);
					quit = 1;
					break;

			}
			if (quit) break;
		}

		if (event.what == nullEvt) {
			ProcessQueue();
		}
	}
	CloseWindow(shadow);
	CloseWindow(win); // or just hide so url is retained?
	InitCursor(); /* reset possible I-beam cursor */
}


void DoSave(void) {
	/* For a text window, save to a file... */

	Handle teH;
	SFReplyRec2 sr;

	GrafPortPtr win = FrontWindow();
	if (!win) return;

	if (GetSysWFlag(win)) return;

	teH = (Handle)GetCtlHandleFromID(win, kGopherText);

	if (_toolErr || !teH) return;


	memset(&sr, 0, sizeof(sr));
	sr.nameRefDesc = refIsNewHandle;
	sr.pathRefDesc = refIsNewHandle;

	// todo -- create default file name from the selector.
	SFPutFile2(170, 35, 0, (Ref)"\pSave file as:", 0, (Ref)"\x09\x00text.file", &sr);


	if (sr.good) {
		static NameRecGS destroyDCB = { 1, NULL };
		static CreateRecGS createDCB = { 4, NULL, 0xe3, 0x04, 0x00 };
		static OpenRecGS openDCB = { 4, 0, NULL, writeEnable, 0 };
		static RefNumRecGS closeDCB = { 1, 0 };
		static IORecGS ioDCB = { 4, 0, NULL, 0, 0 };

		Handle h;
		Handle textH = 0;
		GSString255 *name;
		word error = 0;

		WaitCursor();

		/*
		TEGetText normally returns the full text size;
		bufferDesc | 0x20 will return only the selected text.
		*/ 
		ioDCB.requestCount = TEGetText(0b11101, (Ref)&textH, 0, 0, 0, teH);
		HLock(textH);
		ioDCB.dataBuffer = *(void **)textH;

		h = (Handle)sr.pathRef;
		HLock(h);
		name = &(*(ResultBuf255 **)h)->bufString;
		destroyDCB.pathname = name;
		createDCB.pathname = name;
		openDCB.pathname = name;

		DestroyGS(&destroyDCB);
		CreateGS(&createDCB);
		if (_toolErr) {
			error = _toolErr;
			goto fini;
		}

		OpenGS(&openDCB);
		if (_toolErr) {
			error = _toolErr;
			goto fini;
		}
		closeDCB.refNum = openDCB.refNum;
		ioDCB.refNum = openDCB.refNum;

		WriteGS(&ioDCB);
		if (_toolErr) error = _toolErr;
		CloseGS(&closeDCB);

fini:
		DisposeHandle(textH);
		DisposeHandle((Handle)sr.nameRef);
		DisposeHandle((Handle)sr.pathRef);
		InitCursor();

		if (error) {
			ErrorWindow(0, NULL, error);
		}
	}

}

#define kPageUp 't'
#define kPageDown 'y'
#define kHome 's'
#define kEnd 'w'

#pragma toolparms 1
#pragma databank 1

enum {
	opNormal = 0,
	opNothing = 2,
	opReplaceTest = 4,
	opMoveCursor = 6,
	opExtendCursor = 8,
	opCut = 10,
	opCopy = 12,
	opPaste = 14,
	opClear = 16
};

static pascal void KeyFilter(Handle teH, unsigned type) {
	if (type == 1) {
		// 
		TERecord *te = *(TERecord **)teH;
		unsigned c = te->theKeyRecord.theChar;
		unsigned mod = te->theKeyRecord.theModifiers;

		if (mod & keyPad) c |= 0x80;
		if (c == ' ' || c == kPageUp|0x80 || c == kPageDown|0x80 || c == kHome|0x80 || c == kEnd|0x80) {
			te->theKeyRecord.theOpCode = opNothing;
			PostEvent(app4Evt, c);
		}
		else {
			te->theKeyRecord.theOpCode = opNormal;
		}
	}
}

#pragma toolparms 0
#pragma databank 0


void NewTextWindow(text_cookie *cookie, void *text, unsigned long length) {

	static TEStyle style = {
		// 0x09000004, // font id
		{ FixedFontID, 0, FixedFontSize},
		0x0000, 0xffff, // fore gound, back ground
		0x0000 // user data
	};

	GrafPortPtr win;
	Handle teH;

	win = NewWindow2(cookie->title, (Long)cookie, WindowDrawControls, NULL, refIsResource, kTextWindow, rWindParam1);
	if (_toolErr) {
		return;
	}

	teH = (Handle)GetCtlHandleFromID(win, kGopherText);

	(**((TERecord **)teH)).textFlags &= ~fReadOnly;

	TEStyleChange(teReplaceFont | teReplaceSize|teReplaceAttributes, &style, teH);
	TESetText(teDataIsTextBlock | teTextIsPtr, (Ref)text, length, 0, NULL, teH);

	(**((TERecord **)teH)).textFlags |= fReadOnly;
	(**((TERecord **)teH)).keyFilter = (ProcPtr)KeyFilter;

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
		DrawIcon(iconPtr, 0, rectPtr->h1 + 2, rectPtr->v1 + 1);

	SetForeColor(0x0000);
	SetTextMode(0);
	MoveTo(rectPtr->h1 + 32, rectPtr->v2 - 2);

#if 0
	if (memFlags & memDisabled) {
		SetPenMask(DimMask);
	}
#endif


	if (FixedFont) SetFont((FontHndl)FixedFont);
	DrawStringWidth(dswNoCondense | dswTruncRight | dswPString | dswStrIsPtr, (Ref)entry->name, width);
	if (FixedFont) SetFont((FontHndl)SystemFont);

#if 0
	if (memFlags & memDisabled) {
		SetPenMask(DimMask);
		EraseRect(rectPtr);
		SetPenMask(NorMask);
	}
#endif
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

void DoSelectAll(void) {
	Handle teH;

	GrafPortPtr win = FrontWindow();
	if (!win) return;

	if (GetSysWFlag(win)) return;

	teH = (Handle)GetCtlHandleFromID(win, kGopherText);

	if (_toolErr || !teH) return;

	TESetSelection((Pointer)0, (Pointer)-1, teH);

}

ListEntry *SelectedIndex(ListCtlRec *rec) {

	ListEntry *e = (ListEntry *)rec->ctlList;
	unsigned count = rec->ctlData >> 16;

	unsigned i;

	for (i = 0; i < count; ++i, ++e) {
		if (e->flags & memSelected) {
			return e;
		}
	}
	return NULL;
}

void DoCopy(void) {
	// TaskMaster manager copy from a Text window; this is for copying the
	// selected URL from an index window.

	GrafPortPtr win = FrontWindow();
	if (!win) return;

	if (GetSysWFlag(win)) return;

	CtlRecHndl ctrlH = GetCtlHandleFromID(win, kGopherIndex);
	if (_toolErr || !ctrlH) return;

	ListCtlRec *list = *(ListCtlRec **)ctrlH;

	ListEntry *e = SelectedIndex(list);
	if (!e) return;


	char *host = e->host;
	char *selector = e->selector;

	if (!host) return;

	unsigned len;
	len = *host + 2; // host/x
	if (e->port != 70) {
		len += 6; // :65535
	}

	if (selector) {
		len += *selector;
	}


	char *cp = malloc(len);
	if (!cp) return;

	// make a url -
	// host [:port] [/type/selector]

	len = 0;
	memcpy(cp, host + 1, *host);
	len = *host;

	if (e->port != 70) {
		static char buffer[5];
		int i;

		cp[len++] = ':';
		Int2Dec(e->port, buffer, sizeof(buffer), 0);

		for(i = 0; i < 5; ++i) {
			if (buffer[i] != ' ') cp[len++] = buffer[i];
		}
	}
	cp[len++] = '/';
	if (selector || e->type != kGopherTypeIndex) {
		cp[len++] = e->type;
	}
	if (selector) {
		memcpy(cp + len, selector + 1, *selector);
		len += *selector;
	}

	ZeroScrap();
	PutScrap(len, textScrap, cp);
	free(cp);
}

void ToggleWrapText(void) {


	static TEStyle style = {
		// 0x09000004, // font id
		{ FixedFontID, 0, FixedFontSize},
		0x0000, 0xffff, // fore gound, back ground
		0x0000 // user data
	};

	GrafPortPtr win = FrontWindow();


	Handle teH = (Handle)GetCtlHandleFromID(win, kGopherText);
	if (_toolErr || !teH) return;

	TERecord *tr = *(TERecord **)teH;
	if (tr->textFlags & fNoWordWrap) {
		tr->textFlags &= ~fNoWordWrap;
		tr->textFlags &= ~fReadOnly;
		CheckMItem(1, kWrapTextItem);
	} else {
		tr->textFlags |= fNoWordWrap;
		tr->textFlags &= ~fReadOnly;
		SetMItemMark(0, kWrapTextItem);
	}

	// call TEStyleChange to force a re-layout.
	TEStyleChange(teReplaceFont | teReplaceSize|teReplaceAttributes, &style, teH);

	tr = *(TERecord **)teH;
	tr->textFlags |= fReadOnly;
}

void DoMenu(void) {
	unsigned item = event.wmTaskData & 0xffff;
	unsigned menu = event.wmTaskData >> 16;

	HiliteMenu(0, menu);
	switch (item) {
		case kUndoItem:
		case kCutItem:
		case kPasteItem:
		case kClearItem:
			break;

		case kCopyItem:
			DoCopy();
			break;

		case kCloseItem:
			DoCloseWindow(0);
			break;

		case kSelectAllItem:
			DoSelectAll();
			break;

		case kWrapTextItem:
			ToggleWrapText();
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

		case kSaveItem:
			DoSave();
			break;

		case kConnectItem:
			TCPIPConnect(NULL);
			break;

		case kDisconnectItem:
			TCPIPDisconnect(1, NULL);
			break;
	}
}

void OpenIndex(ListCtlRec *list) {
	ListEntry *e = SelectedIndex(list);
	if (!e) return;

	// todo -- if binary file, SFPutFile2 and open for writing.

	if (e->type == kGopherTypeSearch) {
		DoSearch(e);
	} else {
		QueueEntry(e, NULL);
	}
}

void DoIndexKey(GrafPortPtr win) {

	// CtlRecPtr ctrlPtr;
	ListCtlRec *list;
	CtlRecHndl ctrlH = GetCtlHandleFromID(win, kGopherIndex);
	if (_toolErr || !ctrlH) return;

	// ctrlPtr = *ctrlH;

	unsigned c = event.message;
	if (event.modifiers & keyPad) c |= 0x80;

	list = *(ListCtlRec **)ctrlH;
	unsigned total = list->ctlData >> 16;

	switch(c) {
		case 0x0a:
		case 0x0b:
			// up/down
			SetPort(win);
			ListKey(0, &event, ctrlH);
			break;
		#if 0
		case 0x0d:
			OpenIndex((ListCtlRec *)ctrlPtr);
			break;
		#endif
		case ' ':
		case kPageDown|0x80: {
			unsigned index = list->ctlValue + (list->ctlData & 0xffff);
			if (index < total) {
				SetCtlValue(index, ctrlH);
			}
			break;
		}
		case kPageUp|0x80:
			break;
		case kHome|0x80:
			SetCtlValue(1, ctrlH);
			break;
	}

}

void DoTextKey(GrafPortPtr win) {
	TERecord *te;
	unsigned c;

	Handle teH = (Handle)GetCtlHandleFromID(win, kGopherText);
	if (_toolErr || !teH) return;

	te = *(TERecord **)teH;
	c = event.message;

	if (event.modifiers & keyPad) c |= 0x80;

	if (c == ' ' || c == (kPageDown|0x80)) {
		// page down
		int h = te->viewRect.v2 - te->viewRect.v1;
		h /= te->vertScrollAmount;
		TEScroll(teScrollRelUnit, h, 0, teH);
	}
	if (c == (kPageUp|0x80)) {
		int h = te->viewRect.v2 - te->viewRect.v1;
		h /= te->vertScrollAmount;
		TEScroll(teScrollRelUnit, -h, 0, teH);	
	}
	if (c == (kHome|0x80)) {
		TEScroll(teScrollAbsUnit, 0, 0, teH);
	}
	if (c == (kEnd|0x80)) {
		TEScroll(teScrollAbsUnit, 0x7fffffff, 0, teH);
	}
}

// app4Event via Text Edit key filter.
void DoTextKey2(void) {

	TERecord *te;
	unsigned c;


	GrafPortPtr win = FrontWindow();
	if (_toolErr || !win) return;

	Handle teH = (Handle)GetCtlHandleFromID(win, kGopherText);
	if (_toolErr || !teH) return;

	te = *(TERecord **)teH;
	c = event.message;

	int h = te->viewRect.v2 - te->viewRect.v1;


	switch(c) {
	case ' ':
	case kPageDown|0x80:
		h /= te->vertScrollAmount;
		TEScroll(teScrollRelUnit, h, 0, teH);	
		break;

	case kPageUp|0x80:
		h /= te->vertScrollAmount;
		TEScroll(teScrollRelUnit, -h, 0, teH);	
		break;

	case kEnd|0x80:
		TEScroll(teScrollAbsUnit, 0x7fffffff, 0, teH);
		break;

	case kHome|0x80:
		TEScroll(teScrollAbsUnit, 0, 0, teH);
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
		case app4Evt:
			DoTextKey2();
			break;

		case autoKeyEvt:
		case keyDownEvt: {
			// keyboard navigation for index?

			cookie *c;
			GrafPortPtr win = (GrafPortPtr)FrontWindow();
			if (!win) break;
			if (GetSysWFlag(win)) break; // NDA, etc.

			c = (cookie *)GetWRefCon(win);
			if (!c) break;
			if (c->type == kGopherTypeIndex) {
				DoIndexKey(win);
			} else {
				DoTextKey(win);
			}
			break;
		}

		case mouseDownEvt:
			break;

		case nullEvt:
			ProcessQueue();
			break;

		}
		WindowChange();
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
