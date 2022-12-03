
/*
 * index / list manager stuff.
 *
 *
 *
 * list member rec:
 * void * string
 * byte memFlag (0x80 = selected, 0x40 = disabled)
 * listMemSize -5 additional bytes.
 *
 * need to know
 * 1. display name
 * 2. type (byte)
 * 3. url
 *
 */


/*
* index: type-code display-name <TAB> selector <TAB> host <TAB> port
*
*/

#include <control.h>
#include <list.h>
#include <memory.h>
#include <qdaux.h>
#include <quickdraw.h>

#include <ctype.h>

extern unsigned MyID;

typedef struct GopherMemRec {
	char *name;
	unsigned char memFlags;
	unsigned char type;
	unsigned port;
	char *selector;
	char *host;
} GopherMemRec;

// size = 16
// static_assert(16 == sizeof(struct GopherMemRec));

// since server is likely to be identical for most if not all entries, unique it.
// ummm we can just use the line.

#pragma databank 1 
#pragma toolparms 1
pascal void ListDraw(Rect *rectPtr, GopherMemRec *ptr, Handle ctrl) {

	static unsigned char DimMask[] = {
		0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa,
	};

	static unsigned char NorMask[] = {
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	};

	void *iconPtr = nil;
	unsigned width = rectPtr->h2 - rectPtr->h1 - 10;

	// draw type icon and name.
	unsigned memFlags = ptr->memFlags;

	// fill rect (white or green)
	if (memFlags & memSelected)
		SetBackColor(0xaaaa); // green
	else
		SetBackColor(0xffff); // white
	EraseRect(rectPtr);

	if (iconPtr)
		DrawIcon(iconPtr, 0, rectPtr->h1, rectPtr->v2);

	SetForeColor(0x0000);
	SetTextMode(0);
	MoveTo(rectPtr->h1 + 10, rectPtr->v2);

	if (memFlags & memDisabled) {
		SetPenMask(DimMask);
	}

	DrawStringWidth(dswTruncRight + dswCString + dswStrIsPtr, (Ref)ptr->name, width);

	if (memFlags & memDisabled) {
		SetPenMask(NorMask);
	}

}

#pragma databank 0
#pragma toolparms 0

Handle IndexHandle;
unsigned IndexCount;
unsigned IndexReserved;
Handle IndexControl;

void parse_begin(void) {
	/* clear the index */
	NewList2(NULL, 1, NULL, refIsPointer, 0, IndexControl);
	IndexCount = 0;
	if (!IndexHandle) {
		IndexHandle = NewHandle(16 * sizeof(struct GopherMemRec), MyID, attrNoSpec | attrLocked, 0);
		if (_toolErr) {
			IndexHandle = 0;
			return;
		}
		IndexReserved = 16;
	}
}

void parse_end(void) {

	/* update the list with new values */

	HLock(IndexHandle);
	NewList2((Pointer)ListDraw, 1, (Ref)IndexHandle, refIsHandle, IndexCount, IndexControl);
}

// return -1 on memory error.
int parse_index(char *str) {

	struct GopherMemRec *rec;

	unsigned type = 0;
	unsigned port = 0;
	unsigned c;
	unsigned i;

	char *name = NULL;
	char *selector = NULL;
	char *host = NULL;

	type = *str;
	if (type == 0) return 0;

	// name
	name = str + 1;
	for (i = 1; ; ++i) {
		c = str[i];
		if (c < 0x20) break;
	}
	if (type == 'i') {
		// informational -- no selector / host /port
		str[i] = 0;
		if (!*name) return 0; // ?
		goto store;
	}

	if (c != 0x09) return 0; // ?
	str[i++] = 0;

	// selector
	selector = str + i;
	for(;;++i) {
		c = str[i];
		if (c < 0x20) break;
	}
	if (c != 0x09) return 0;
	str[i++] = 0;

	// host
	host = str + i;
	for(;;++i) {
		c = str[i];
		if (c < 0x20) break;
	}
	if (c != 0x09) return 0;
	str[i++] = 0;

	// port
	port = 0;
	for(;;++i) {
		c = str[i];
		if (c < 0x20) break;
		if (!isdigit(c)) { port = 70; break; }
		port = port * 10 + (c & 0x0f);
	}

store:

	if (IndexCount == IndexReserved) {
		HUnlock(IndexHandle);
		IndexReserved += 16;
		SetHandleSize(IndexReserved * sizeof(struct GopherMemRec), IndexHandle);
		if (_toolErr) return -1;
		HLock(IndexHandle);
	}
	rec = *(GopherMemRec **)IndexHandle + IndexCount;


	rec->name = name;
	rec->memFlags = type == 'i' ? memDisabled : 0;
	rec->type = type;
	rec->port = port;
	rec->selector = selector;
	rec->host = host;

	++IndexCount; 
	return 0;
}
