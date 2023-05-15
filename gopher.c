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
#include "hierarchic.h"

unsigned Quit = 0;


enum {
	kThemeStandard = 0,
	kThemeNeXT,
	kThemeMac
};

unsigned Theme = kThemeStandard;
char SearchURL[256];

const char *ReqName = "\pTCP/IP~kelvin~gopher~";

unsigned MyID;

MenuRecHndl hMenu = NULL;
MenuRecHndl hTabMenu = NULL;


static Pointer Icons[5];

#define kMaxWindows 16


GrafPortPtr Windows[kMaxWindows];
unsigned WindowBits = 0;

unsigned MenuWidth = 0;



unsigned EditSearchURL(void);

void SetStringByID(char *, GrafPortPtr, Long);

#pragma toolparms 1
#pragma databank 1

pascal void WindowDrawControls(void) {
	DrawControls(GetPort());
}

static unsigned EndDialog = 0;
static void SimpleBeepProc(WmTaskRec *event) {
	EndDialog = 1;
}

void SimpleEventHook(WmTaskRec *event) {
	if (!event) return;

	unsigned what = event->what;
	unsigned message = event->message;
	if (what == keyDownEvt || what == autoKeyEvt) {

		if (message = 0x1b) EndDialog = 1; /* escape */
		if (event->modifiers & appleKey) {

			switch(message) {
			case 'q':
			case 'Q':
				Quit = 1;
			case '.':
			case 'w':
			case 'W':
				EndDialog = 1;
			}
		}
	}
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
		EnableMItem(kSearchItem);
		EnableMItem(kDisconnectItem);
		DisableMItem(kConnectItem);
	} else {
		DisableMItem(kOpenItem);
		DisableMItem(kSearchItem);
		DisableMItem(kDisconnectItem);
		EnableMItem(kConnectItem);
	}
}


void TabMenuUpdate(unsigned flags) {

	// for hierarchic, we need to insert the menu to use CheckMItem, etc.

	if (hTabMenu) {
		InsertMenu(hTabMenu, 0);
	}

	if (flags & kFlagTab) {
		CheckMItem(1, kTab4Item);
		CheckMItem(0, kTab8Item);
	} else {
		CheckMItem(0, kTab4Item);
		CheckMItem(1, kTab8Item);
	}

	if (hTabMenu) {
		DeleteMenu(kH_TabItem);
	}
}
// todo -- update for DAs.
// todo -- copy should only be active if there's a text selection or list selection.

void MenuUpdate(struct cookie *cookie) {

	static int prevMItem = -1;

	if (prevMItem >= 0) {
		CheckMItem(0, prevMItem);
		prevMItem = -1;
	}

	if (!cookie) {

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

		CheckMItem(0, kWrapTextItem);
		SetMenuFlag(disableMenu, kTextMID);
		HiliteMenu(0, kTextMID);

		DisableMItem(kAddBookmarkItem);

		return;
	}

	unsigned type = cookie->type;
	unsigned flags = cookie->flags;

	if (type == kGopherTypeIndex) {

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

		CheckMItem(0, kWrapTextItem);
		SetMenuFlag(disableMenu, kTextMID);
		HiliteMenu(0, kTextMID);

		EnableMItem(kAddBookmarkItem);

	} else {
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

		CheckMItem(flags & kFlagWrap, kWrapTextItem);
		SetMenuFlag(enableMenu, kTextMID);
		HiliteMenu(0, kTextMID);

		TabMenuUpdate(flags);

		EnableMItem(kAddBookmarkItem);
	}

	prevMItem = cookie->menuID + WindowBase;
	CheckMItem(1, prevMItem);
}

void WindowChange(void) {

	static GrafPortPtr PrevWin = (GrafPortPtr)-1;

	struct cookie *c;
	GrafPortPtr win = FrontWindow();
	if (win == PrevWin) return;
	PrevWin = win;

	if (!win) {
		MenuUpdate(NULL);
	}

	if (GetSysWFlag(win)) {
		// DA window.
		return;
	}

	c = (struct cookie *)GetWRefCon(win);
	MenuUpdate(c);
}



#pragma toolparms 1
#pragma databank 1
pascal word MenuProc(word message, MenuRecHndl menuRecH, Rect *rectPtr, word xHit, word yHit, word param) {
	word rv = 0;
	MenuRec *menu = *menuRecH;

#if 0
	static struct {
		unsigned type;
		unsigned size;
		unsigned height;
		unsigned width;
		unsigned char image[6];
		unsigned char mask[6];
	} icon = {
		0x8000,
		6, 3, 4,
		{
			0xc0, 0x3f,
			0x00, 0x0f,
			0xc0, 0x3f
		},
		{
			0xff, 0xff,
			0xff, 0xff,
			0xff, 0xff
		}
	};
#else
	static struct {
		unsigned type;
		unsigned size;
		unsigned height;
		unsigned width;
		unsigned char image[24];
		unsigned char mask[24];
	} icon = {
		0x8000,
		24, 8, 6,
		{
			#if 0
			0b11110011, 0b11111111, 0b11111111,
			0b11000000, 0b11111111, 0b11001111,
			0b00000000, 0b00111111, 0b11001111,
			0b11110011, 0b11111111, 0b11001111,
			0b11110011, 0b11111111, 0b11001111,
			0b11110011, 0b11111100, 0b00000000,
			0b11110011, 0b11111111, 0b00000011,
			0b11111111, 0b11111111, 0b11001111,
			#endif
			0xF3, 0xFF, 0xFF, 
			0xC0, 0xFF, 0xCF, 
			0x00, 0x3F, 0xCF, 
			0xF3, 0xFF, 0xCF, 
			0xF3, 0xFF, 0xCF, 
			0xF3, 0xFC, 0x00, 
			0xF3, 0xFF, 0x03, 
			0xFF, 0xFF, 0xCF,
		},
		{
			0xFF, 0xFF, 0xFF, 
			0xFF, 0xFF, 0xFF, 
			0xFF, 0xFF, 0xFF, 
			0xFF, 0xFF, 0xFF, 
			0xFF, 0xFF, 0xFF, 
			0xFF, 0xFF, 0xFF, 
			0xFF, 0xFF, 0xFF, 
			0xFF, 0xFF, 0xFF,
		}
	};
#endif




	unsigned x;

	switch(message) {
	case mDrawMsg:
		/* draw the menu */
		break;
	case mChooseMsg:
		/* return selected menu item */
		break;
	case mSizeMsg:
		/* calculate menu size */
		break;
	case mDrawTitle:

		EraseRect(rectPtr);
		x = (unsigned)menu->titleName;
		if (x) {
			DrawIcon((Pointer)&icon, 0, rectPtr->h1 + 4, rectPtr->v1 + 2);
		}
		rv = 1; /* prevent normal title drawing code */
		break;
	case mDrawMItem:
		/* draw a menu item */
		break;
	case mGetMItemID:
		break;
	}

	return rv;
}
#pragma toolparms 0
#pragma databank 0

static MenuRec *ActivityMenu;
void UpdateActivityMenu(unsigned status) {

	ActivityMenu->titleName = (Pointer)status;
	HiliteMenu(0, ActivityMenu->menuID);
}

MenuRecHndl MakeActivityMenu(void) {
	MenuRecHndl h;
	MenuRec *menu;

	h = (MenuRecHndl)NewHandle(sizeof(MenuRec), MyID, attrLocked|attrFixed|attrNoSpec, 0);
	if (h) {
		menu = *h;
		memset(menu, 0, sizeof(MenuRec));

		// menuCustom means an empty menu will always draw.
		// 
		menu->menuID = 9;
		menu->menuProc = MenuProc;
		menu->menuFlag = /*menuCustom | */ menuDisabled | menuAllowCache;
		menu->titleWidth = 20;
		menu->titleName = (Pointer)0;
		ActivityMenu = menu;
	}
	return h;
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
// #define FixedFontID 32732
// #define FixedFontSize 8

FontID FixedFontID = { { monaco, 0, 9 } };

TEStyle FixedStyle = {
	// 0x09000004, // font id
	{ monaco, 0, 9}, // font
	0x0000, 0xffff, // fore gound, back ground
	0x0000 // user data
};

// monaco 9
word FixedCharWidth = 6;
word FixedCharHeight = 11;


struct TEPixelRuler {
   Word leftMargin;
   Word leftIndent;
   Word rightMargin;
   Word just;
   Word extraLS;
   Word flags;
   LongWord userData;
   Word tabType;
   Word tabTerminator;
} FixedRuler = {
	0, 0, 640, leftJust,
	0, 0, 0, stdTabs, 6 * 8
};


// should also calculate width of 80-column text.
#undef family // control.h
struct FontHeader {
	unsigned offsetToMF;
	unsigned family;
	unsigned style;
	unsigned size;
	unsigned version;
	unsigned fbrExtent;
};


struct MacFontHeader {
	unsigned fontType;
	unsigned firstChar;
	unsigned lastChar;
	unsigned widMax;
	unsigned kernMax;
	unsigned nDescent;
	unsigned fRectWidth;
	unsigned fRectHeight;
	unsigned owTLoc;
	unsigned ascent;
	unsigned descent;
	unsigned leading;
	unsigned rowWords;
	// bitmapImage
	// locTable
	// owTable
};

word FindUnusedFontID(void) {
	unsigned i;

	for (i = 32768; ; ++i) {
		word stats = GetFamInfo(i, NULL);
		if (stats & notFoundBit) return i;
	}
}

Handle LoadFixedFont(void) {
	Handle h;
	unsigned char *ptr;
	unsigned long size;
	struct FontHeader *fh;
	struct MacFontHeader *mfh;
	int offset;

	unsigned newFamily = FindUnusedFontID();

	h = LoadResource(rFont, 1);
	if (_toolErr) return NULL;
	DetachResource(rFont, 1);

	HLock(h);
	ptr = *(unsigned char **)h;
	offset = *ptr + 1;
	fh = (struct FontHeader *)(ptr + offset);
	mfh = (struct MacFontHeader *)((char *)fh + fh->offsetToMF);
	fh->family = newFamily;

	FixedFontID.fidRec.famNum = newFamily;
	FixedFontID.fidRec.fontStyle = 0;
	FixedFontID.fidRec.fontSize = fh->size;

	FixedStyle.styleFontID.fidLong = FixedFontID.fidLong;

	FixedCharHeight = mfh->fRectHeight;
	FixedCharWidth = mfh->fRectWidth;

	FixedRuler.tabTerminator = 8 * FixedCharWidth;

	// fh->fbrExtent == width


	AddFamily(newFamily, ptr);
	// remove the name...

	size = GetHandleSize(h) - offset; // skip pascal name

	BlockMove(ptr + offset, ptr, size);
	SetHandleSize(size, h);
	AddFontVar((FontHndl)h, 0);
	return h;
}

Handle LoadResourceFont(unsigned familyID, unsigned size) {
	Handle h;
	FontID FF = {{ familyID, 0, size } };
	FontStatRec fsr;

	FindFontStats(FF, 0, 1, &fsr);
	if (_toolErr || (fsr.resultStats & notFoundBit)) {

		unsigned char *ptr;
		int offset;
		unsigned long resID = (familyID << 8) | size;

		h = LoadResource(rFont, resID);
		if (_toolErr) return NULL;
		DetachResource(rFont, resID);

		HLock(h);
		ptr = *(unsigned char **)h;
		offset = *ptr + 1;

		AddFamily(familyID, ptr);

		size = GetHandleSize(h) - offset; // skip pascal name

		BlockMove(ptr + offset, ptr, size);
		SetHandleSize(size, h);
		AddFontVar((FontHndl)h, 0);
		return h;

	}
	InstallFont(FF, 0);
	if (_toolErr) return NULL;
	return (Handle)GetFont();
}

static void ApplyTheme(void) {

	static ColorTable GreyScale = {
		0x0000, 0x0555, 0x0aaa, 0x0fff,
		0x0000, 0x0555, 0x0aaa, 0x0fff,
		0x0000, 0x0555, 0x0aaa, 0x0fff,
		0x0000, 0x0555, 0x0aaa, 0x0fff,
	};

	static ColorTable Standard = {
		0x0000, 0x0f00, 0x00f0, 0x0fff,
		0x0000, 0x000f, 0x0ff0, 0x0fff,
		0x0000, 0x0f00, 0x00f0, 0x0fff,
		0x0000, 0x000f, 0x0ff0, 0x0fff,
	};

	GrafPortPtr oldPort;





	// erase left/right/top of the window frame.
	oldPort = GetPort();
	SetPort(GetMenuMgrPort());
	SetPenSize(2, 1);
	SetSolidPenPat(0x0f);
	MoveTo(0, 11);
	LineTo(0, 0);
	LineTo(639-1, 0);
	LineTo(639-1, 11);
	SetPort(oldPort);

	/* next theme! */
	if (Theme == kThemeNeXT) {
		SetColorTable(0, GreyScale);
		Desktop(5, 0x400000aa);
	}
	if (Theme == kThemeMac) {
		SetColorTable(0, Standard);
		Desktop(5, 0x400002c3); // b/w checkerboard.
	}
	if (Theme == kThemeStandard) {
		// standard desktop colors.
		SetColorTable(0, Standard);
		Desktop(5, 0x400000d0);
	}


}

static void Setup(void) {

	/* menu bars */

	SetSysBar(NewMenuBar2(2, kMenuBarID, 0));
	SetMenuBar(0);
	FixAppleMenu(kAppleMID);

	if (HierarchicStartUp(MyID)) {
		MenuRecHndl m;

		m = GetMHandle(kTextMID);
		DeleteMenu(kTextMID);
		DisposeMenu(m);

		hMenu = HierarchicNew(refIsResource, kH_TextMID);

		InsertMenu(hMenu, kEditMID);

		hTabMenu = HierarchicGetMenuHandle(hMenu, kH_TabItem);

	}

	#if 1
	{
		MenuRecHndl m = MakeActivityMenu();
		InsertMenu(m, kWindowMID);
	}
	#endif

	FixMenuBar();
	DrawMenuBar();

#if 0
	/* find the right edge of the menu bar */
	MenuWidth = 0;
	MenuWidth = GetMTitleStart();
	for (i = kAppleMID; i <= kWindowMID; ++i)
		MenuWidth += GetMTitleWidth(i);
#endif

	InitCursor();
	ApplyTheme();

	StartupQueue();

	Icons[0] = (Pointer)GetIcon(kIconText); // text
	Icons[1] = (Pointer)GetIcon(kIconFolder); // folder
	Icons[2] = (Pointer)GetIcon(kIconBinary); // binary.
	Icons[3] = (Pointer)GetIcon(kIconDocument); // document
	Icons[4] = (Pointer)GetIcon(kIconSearch);


	WindowBits = 0;

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


	memcpy(SearchURL, "\pgopher.floodgap.com/7/v2/vs", 28);

}

Pointer IconForType(unsigned type) {
	switch(type) {
		case kGopherTypeText: return Icons[0]; /* text */
		case kGopherTypeIndex: return Icons[1]; /* directory/index */
		case kGopherTypeBinary: return Icons[2]; /* binary */
		case kGopherTypeSearch: return Icons[4];
		case kGopherTypeInfo: /* informational */
		case kGopherTypeError: /* error */
			return NULL;
		default: return Icons[3];
	}
}

static WmTaskRec event;



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
	if (!event) return;

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

	GrafPortPtr win;
	GrafPortPtr shadow;
	long id;

	win = NewWindow2(NULL, NULL, WindowDrawControls, NULL, refIsResource, kAboutWindow, rWindParam1);

	EndDialog = 0;

	shadow = CreateShadowWindow(win);
	for(;;) {
		#define flags mwUpdateAll | mwDeskAcc
		id = DoModalWindow(&event, NULL, (VoidProcPtr)SimpleEventHook, (VoidProcPtr)SimpleBeepProc, flags);
		#undef flags

		if (EndDialog) break;
		if (id == kAboutButton) break;

		ProcessQueue();
	}

	CloseWindow(shadow);
	CloseWindow(win);
}

void DoPrefs(void) {

	unsigned oldTheme;
	static char oldSearchURL[256];

	GrafPortPtr win;
	GrafPortPtr shadow;
	long id;
	// Handle h;
	unsigned changes = 0;


	oldTheme = Theme;
	memcpy(oldSearchURL, SearchURL, 256);


	win = NewWindow2(NULL, NULL, WindowDrawControls, NULL, refIsResource, kPrefsWindow, rWindParam1);

	EndDialog = 0;

	SetCtlValueByID(0x100 | Theme, win, kPrefsTheme);
	SetStringByID(SearchURL, win, kPrefsSearch);

	shadow = CreateShadowWindow(win);
	for(;;) {
		#define flags mwUpdateAll | mwDeskAcc
		id = DoModalWindow(&event, NULL, (VoidProcPtr)SimpleEventHook, (VoidProcPtr)SimpleBeepProc, flags);
		#undef flags

		if (EndDialog) break;

		if (id == kPrefsSave)
			break;

		if (id == kPrefsCancel) {
			if (changes & 1) {
				Theme = oldTheme;
				ApplyTheme();
			}
			if (changes & 2) {
				memcpy(SearchURL, oldSearchURL, 256);
			}
			break;
		}

		if (id == kPrefsTheme) {
			CtlRecHndl h = (CtlRecHndl)event.wmTaskData2;
			Theme = GetCtlValue(h) & 0x0f;
			ApplyTheme();
			changes |= 1;
		}

		if (id == kPrefsSearch) {
			if (EditSearchURL()) {
				changes |= 2;
				SetStringByID(SearchURL, win, kPrefsSearch);
			}
		}

		ProcessQueue();
	}

	CloseWindow(shadow);
	CloseWindow(win);

}



// for use with string control, which uses a pstring.
void SetStringByID(char *text, GrafPortPtr win, Long id) {

	CtlRecHndl ctrlH = GetCtlHandleFromID(win, id);
	if (!_toolErr) {
		unsigned moreFlags = GetCtlMoreFlags(ctrlH);
		unsigned mf = (moreFlags & ~0x03) | refIsPointer;
		if (mf != moreFlags)SetCtlMoreFlags(mf, ctrlH);
		SetCtlTitle(text,(Handle)ctrlH);
	}
}


void DoNetworkHelper(GrafPortPtr win) {
	long ip;
	static char buffer[16];

	if (TCPIPGetConnectStatus()) {
		ip = TCPIPGetMyIPAddress();
		TCPIPConvertIPToASCII(ip, buffer, 0);
		SetStringByID("\pConnected", win, kNetworkStatusRight);
		SetStringByID(buffer, win, kNetworkIPRight);

		HiliteCtlByID(inactiveHilite, win, kNetworkConnect);
		HiliteCtlByID(noHilite, win, kNetworkDisconnect);

	} else {
		SetStringByID("\pDisconnected", win, kNetworkStatusRight);
		SetStringByID("\p", win, kNetworkIPRight);

		HiliteCtlByID(noHilite, win, kNetworkConnect);
		HiliteCtlByID(inactiveHilite, win, kNetworkDisconnect);
	}
}

#pragma databank 1
#pragma toolparms 1

void NetworkStatusCallback(const char *str) {
	SetStringByID(str, NULL, kNetworkStatusRight);
}

#pragma databank 0
#pragma toolparms 0

void DoNetwork(void) {


	GrafPortPtr win;
	GrafPortPtr shadow;
	unsigned id;


	EndDialog = 0;

	win = NewWindow2(NULL, NULL, WindowDrawControls, NULL, refIsResource, kNetworkWindow, rWindParam1);

	shadow = CreateShadowWindow(win);

	DoNetworkHelper(win);

	for(;;) {
		#define flags mwUpdateAll | mwDeskAcc
		id = DoModalWindow(&event, NULL, (VoidProcPtr)SimpleEventHook, (VoidProcPtr)SimpleBeepProc, flags);
		#undef flags

		if (EndDialog) break;

		if (id == kNetworkConnect) {
			WaitCursor();
			TCPIPConnect(NetworkStatusCallback);
			InitCursor();
			DoNetworkHelper(win);
		}
		if (id == kNetworkDisconnect) {
			WaitCursor();
			TCPIPDisconnect(1, NetworkStatusCallback);
			InitCursor();
			DoNetworkHelper(win);
		}

		ProcessQueue();
	}

	CloseWindow(shadow);
	CloseWindow(win);
}


unsigned EditSearchURL(void) {

	static char text[256];
	// static URLComponents uc;

	CtlRecHndl ctrlH;
	LERecHndl leH;
	unsigned ok = 0;
	int type;

	GrafPortPtr win = NewWindow2(NULL, NULL, WindowDrawControls, NULL, refIsResource, kURLWindow, rWindParam1);

	GrafPortPtr shadow = CreateShadowWindow(win);

	ctrlH = GetCtlHandleFromID(win, kGopherURL);
	leH = (LERecHndl)GetCtlTitle(ctrlH);

	LESetText(SearchURL + 1, *SearchURL, leH);

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

					type = AnalyzeURL(text);
					if (type == kGopherTypeSearch) {

						memcpy(SearchURL, text, text[0]+1);
						ok = 1;
						quit = 1;
						break;
					}
					// should probably do a batter job of warning...
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
	return ok;
}


void DoOpen(void) {

	static char text[256];
	// static URLComponents uc;

	CtlRecHndl ctrlH;
	LERecHndl leH;
	unsigned ok;


	if (WindowBits == 0xffff) return;

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

					// TODO -- should indicate if it's a binary item
					// so we can save it to a file....
					ok = QueueURL(text, NULL);
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


/*
 * return a search string / null if canceled.
 */
char *SearchPrompt(char *name) {

	static char text[256];
	// static URLComponents uc;

	CtlRecHndl ctrlH;
	LERecHndl leH;

	// GrafPortPtr shadow = NewWindow2(NULL, NULL, WindowDrawControls, NULL, refIsResource, kSearchWindowShadow, rWindParam1);
	GrafPortPtr win = NewWindow2(NULL, NULL, WindowDrawControls, NULL, refIsResource, kSearchWindow, rWindParam1);

	GrafPortPtr shadow = CreateShadowWindow(win);

	ctrlH = GetCtlHandleFromID(win, kSearchLineEdit);
	leH = (LERecHndl)GetCtlTitle(ctrlH);

	if (name && *name) {
		// we can't just set the title, we also have to swap from resource to pointer and set the length.
		CtlRecHndl ctrlH = GetCtlHandleFromID(win, kSearchText);
		SetCtlMoreFlags(fCtlProcRefNotPtr | refIsPointer, ctrlH);
		SetCtlValue(*name, ctrlH);
		SetCtlTitle(name+1, (Handle)ctrlH);
	}

	unsigned quit = 0;
	for(;;) {
		#define flags mwUpdateAll | mwDeskAcc | mwIBeam
		// DoModalWindow(&event, NULL, (VoidProcPtr)0x80000000, (VoidProcPtr)-1, flags);
		DoModalWindow(&event, NULL, (VoidProcPtr)OpenEventHook, (VoidProcPtr)-1, flags);
		#undef flags
		// break on return, esc (apple-. converted to esc)

		if (event.what == app4Evt) {

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
					quit = 2;
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

	if (quit == 2) {
		return text;
	}
	return NULL;
}

GSString255 *FileName(const char *path, word length) {
	/* given a p-string path, generate an appropriate name to save as */

	unsigned i;
	GSString255 *name;

	if (!path || !length) return NULL;

	// path is a pstring so offset by 1.
	for (i = length; i; --i) {
		if (path[i - 1] == '/') break;
	}
	int nlen = length - i;

	name = (GSString255 *)malloc(2 + nlen);
	if (!name) return NULL;
	name->length = nlen;
	memcpy(name->text, path + i, nlen);
	return name;
}

// from the Search... menu.
void DoSearch(void) {

	if (!SearchURL[0]) return;

	char *q = SearchPrompt("\pSearch");
	if (q)
		QueueURL(SearchURL, q);
}



/*
 * return a file descriptor, 0 if canceled.
 */
int FilePrompt(GSString255 *name, unsigned ftype, unsigned atype) {

	static NameRecGS destroyDCB = { 1, NULL };
	static CreateRecGS createDCB = { 4, NULL, 0xe3, 0x04, 0x00 };
	static OpenRecGS openDCB = { 4, 0, NULL, writeEnable, 0 };

	SFReplyRec2 sr;
	Handle h;
	word rv = 0;
	word error = 0;

	memset(&sr, 0, sizeof(sr));
	sr.nameRefDesc = refIsNewHandle;
	sr.pathRefDesc = refIsNewHandle;

	if (!name) name = (GSString255 *)"\x04\x00" "file";
	// todo -- create default file name from the selector.
	SFPutFile2(170, 35, 0, (Ref)"\pSave file as:", 0, (Ref)name, &sr);

	if (_toolErr || !sr.good) return 0;

	createDCB.fileType = ftype;
	createDCB.auxType = atype;

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
	rv = openDCB.refNum;


fini:
	DisposeHandle((Handle)sr.nameRef);
	DisposeHandle((Handle)sr.pathRef);

	if (error) {
		ErrorWindow(0, NULL, error);
	}
	return rv;
}

void DoBinscii(void) {

	Handle teH;
	long size;

	GrafPortPtr win = FrontWindow();
	if (!win) return;

	if (GetSysWFlag(win)) return;

	teH = (Handle)GetCtlHandleFromID(win, kGopherText);

	if (_toolErr || !teH) return;

	Handle textH = 0;
	size = TEGetText(0b11101, (Ref)&textH, 0, 0, 0, teH);
	HLock(textH);
	DecodeBINSCII(*(uint8_t **)textH, size);
	DisposeHandle(textH);
}

void DoSave(void) {
	/* For a text window, save to a file... */

	static RefNumRecGS closeDCB = { 1, 0 };
	static IORecGS ioDCB = { 4, 0, NULL, 0, 0 };

	Handle teH;

	GrafPortPtr win = FrontWindow();
	if (!win) return;

	if (GetSysWFlag(win)) return;

	teH = (Handle)GetCtlHandleFromID(win, kGopherText);

	if (_toolErr || !teH) return;


	// todo - keep the name in the cookie.
	int refNum = FilePrompt(NULL, 0x04, 0x00);
	if (refNum == 0) return;

	Handle textH = 0;

	WaitCursor();

	/*
	TEGetText normally returns the full text size;
	bufferDesc | 0x20 will return only the selected text.
	*/ 
	ioDCB.requestCount = TEGetText(0b11101, (Ref)&textH, 0, 0, 0, teH);
	HLock(textH);
	ioDCB.dataBuffer = *(void **)textH;

	closeDCB.refNum = refNum;
	ioDCB.refNum = refNum;

	WriteGS(&ioDCB);
	if (_toolErr) {
		ErrorWindow(0, NULL, _toolErr);		
	}
	CloseGS(&closeDCB);

	DisposeHandle(textH);
	InitCursor();
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


// update the Windows menu....
void AddWindow(GrafPortPtr win, struct cookie *cookie) {

	unsigned id = 0;
	unsigned mask;
	static MenuItemTemplate template = { 0 };

	for (id = 0, mask = 1; ; ++id, mask <<= 1) {
		if (!(WindowBits & mask)) break;
	}

	template.itemTitleRef = (Ref)cookie->title;
	template.itemID = WindowBase + id;

	cookie->menuID = id;
	Windows[id] = win;
	WindowBits |= mask;

	InsertMItem2(refIsPointer, (Ref)&template, 0xffff, kWindowMID);
	CalcMenuSize(0, 0, kWindowMID);

	if (WindowBits == 1) {
		SetMenuFlag(enableMenu, kWindowMID);
		HiliteMenu(0, kWindowMID);
		// DrawMenuBar causes a noticiable refresh.
		// DrawMenuBar();
	}
}

void RemoveWindow(struct cookie *cookie) {
	unsigned id = cookie->menuID;

	DeleteMItem(id + WindowBase);
	CalcMenuSize(0, 0, kWindowMID);

	Windows[id] = NULL;
	WindowBits &= ~(1 << id);


	if (WindowBits == 0) {
		SetMenuFlag(disableMenu, kWindowMID);
		HiliteMenu(0, kWindowMID);
		// DrawMenuBar();
	}

}

void NewTextWindow(text_cookie *cookie, void *text, unsigned long length) {

	GrafPortPtr win;
	Handle teH;

	win = NewWindow2(cookie->title, (Long)cookie, WindowDrawControls, NULL, refIsResource, kTextWindow, rWindParam1);
	if (_toolErr) {
		return;
	}

	teH = (Handle)GetCtlHandleFromID(win, kGopherText);

	(**((TERecord **)teH)).textFlags &= ~fReadOnly;

	TEStyleChange(teReplaceFont | teReplaceSize|teReplaceAttributes, &FixedStyle, teH);
	TESetRuler(refIsPointer, (Ref)&FixedRuler, teH);
	TESetText(teDataIsTextBlock | teTextIsPtr, (Ref)text, length, 0, NULL, teH);

	(**((TERecord **)teH)).textFlags |= fReadOnly;
	(**((TERecord **)teH)).keyFilter = (ProcPtr)KeyFilter;


	AddWindow(win, (struct cookie *)cookie);

}
#pragma toolparms 1
#pragma databank 1
void pascal ListDraw(Rect *rectPtr, ListEntry *entry, Handle listHandle) {

#if 0
	static unsigned char DimMask[] = {
		0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa,
	};

	static unsigned char NorMask[] = {
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	};
#endif

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

	AddWindow(win, (struct cookie *)cookie);
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
		RemoveWindow(c);
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

#define kTextMarginFudge 30

	GrafPortPtr win = FrontWindow();
	if (!win) return;

	text_cookie *cookie = (text_cookie *)GetWRefCon(win);
	if (!cookie || cookie->type != kGopherTypeText) return;

	Handle teH = (Handle)GetCtlHandleFromID(win, kGopherText);
	if (_toolErr || !teH) return;

	TERecord *tr = *(TERecord **)teH;

	if (cookie->flags & kFlagWrap) {
		tr->textFlags &= ~fGrowRuler;
		tr->textFlags |= fNoWordWrap;
		SetMItemMark(0, kWrapTextItem);
		cookie->flags &= ~kFlagWrap;

		FixedRuler.rightMargin = 640; // doesn't matter....

	} else {

		// fGrowRuler adjusts the right margin based on the 
		// difference in window size so we need to set it here.
		tr->textFlags |= fGrowRuler;
		tr->textFlags &= ~fNoWordWrap;
		CheckMItem(1, kWrapTextItem);
		cookie->flags |= kFlagWrap;

		unsigned w = win->portRect.h2;
		FixedRuler.rightMargin = w - kTextMarginFudge;
	}

	TESetRuler(refIsPointer, (Ref)&FixedRuler, teH);

	// call TEStyleChange to force a re-layout.
	// TEStyleChange(teReplaceFont | teReplaceSize|teReplaceAttributes, &FixedStyle, teH);

	tr = *(TERecord **)teH;
	// tr->textFlags |= fReadOnly;

	FixedRuler.rightMargin = 640;
}

void DoTab(unsigned tab) {

	static struct TEPixelRuler Ruler;

	GrafPortPtr win = FrontWindow();
	if (!win) return;

	text_cookie *cookie = (text_cookie *)GetWRefCon(win);
	if (!cookie || cookie->type != kGopherTypeText) return;

	Handle teH = (Handle)GetCtlHandleFromID(win, kGopherText);
	if (_toolErr || !teH) return;

	unsigned flags = cookie->flags;

	TEGetRuler(refIsPointer, (Ref)&Ruler, teH);

	if (tab == 4) {
		flags |= kFlagTab;
		Ruler.tabTerminator = 4 * FixedCharWidth;
	}
	if (tab == 8) {
		flags &= ~kFlagTab;
		Ruler.tabTerminator = 8 * FixedCharWidth;
	}
	TESetRuler(refIsPointer, (Ref)&Ruler, teH);
	cookie->flags = flags;
	TabMenuUpdate(flags);
}

void DoMenu(void) {
	unsigned item = event.wmTaskData & 0xffff;
	unsigned menu = event.wmTaskData >> 16;

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

		case kBinsciiItem:
			DoBinscii();
			break;

		case kTab4Item:
			DoTab(4);
			break;
		case kTab8Item:
			DoTab(8);
			break;

		case kAbout:
			DoAbout();
			break;
		case kPreferences:
			DoPrefs();
			break;
		case kOpenItem:
			DoOpen();
			break;
		case kSearchItem:
			DoSearch();
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

		case kNetworkStatusItem:
			DoNetwork();
			break;
	}

	// handle window selection.
	if (menu == kWindowMID) {
		unsigned id = item - WindowBase;
		if (id < kMaxWindows) {
			SelectWindow(Windows[id]);
		}
	}

	HiliteMenu(0, menu);
}

void OpenIndex(ListCtlRec *list) {

	if (WindowBits == 0xffff) return;

	ListEntry *e = SelectedIndex(list);
	if (!e) return;

	QueueEntry(e);
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

#if 0
			if (event.wmTaskData4 == kGopherText && (word)event.wmTaskData3 == 0x0084) {
				// check for text edit grow box events.
				WindowResize((GrafPortPtr)event.wmTaskData);
			}
#endif
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
			#if 0
			// text edit uses a control for the grow box
			// so this doesn't apply.
			if ((word)event.wmTaskData == wInGrow) {
				// window has been resized....
				WindowResize();
				break;
			}
			#endif
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
		AlertWindow(awPointer, NULL,
			(Ref)"24~Marinetti 3.0b11 or newer is required.~^Too Bad");
		goto exit;
	}
	// if (TCPIPVersion() & 0x7fff < 0x0300)
	if (TCPIPLongVersion() < 0x03006011) {
		AlertWindow(awPointer, NULL,
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
	if (hMenu) {
		HierarchicDispose(hMenu);
		HierarchicShutDown();
	}
	ShutDownTools(refIsHandle, tlRef);
	MMShutDown(MyID);
	TLShutDown();
	QuitGS(&quitDCB);
}
