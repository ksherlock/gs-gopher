
#pragma noroot
#pragma optimize -1

#include <memory.h>
#include <locator.h>
#include "hierarchic.h"


static asm void __Hierarchic(void) {
	jml 0xffffff
}

static int Hierarchic = 0;

int HierarchicStartUp(unsigned memID) {

	static struct {
		unsigned length;
		char name[15];
	} message = {15 + 2,  "\pBSS:Hierarchic" };

	ResponseRecord r; // message id / create flag
	Handle h;
	unsigned char *ptr;

	Hierarchic = 0;

	r = MessageByName(0, (Pointer)&message);
	if (_toolErr) return 0;

	h = NewHandle(0, memID, 0, 0);
	if (_toolErr) return 0;

	MessageCenter(getMessage, (word)r, h);
	if (_toolErr) goto exit;

	ptr = *(unsigned char **)h;
	asm {
		ldy #0x19
		lda [ptr],y
		xba
		sta __Hierarchic+2
		dey
		dey
		lda [ptr],y
		sta __Hierarchic+1

		lda #1 // StartUp
		jsl __Hierarchic
	}
	Hierarchic = 1;

exit:
	DisposeHandle(h);
	return Hierarchic;
}

MenuRecHndl HierarchicNew(unsigned refDesc, Ref ref) {

	MenuRecHndl rv = 0;
	if (Hierarchic) {
		asm {
			pea 0
			pea 0
			pei refDesc
			pei ref+2
			pei ref
			lda #4 // New
			jsl __Hierarchic
			pla
			sta rv
			pla
			sta rv+2
		}
	}
	return rv;
}

void HierarchicShutDown(void) {

	if (Hierarchic) {
		asm {
			lda #2 // ShutDown
			jsl __Hierarchic
		}
		Hierarchic = 0;
	}
}

void HierarchicDispose(MenuRecHndl menu) {
	if (Hierarchic) {
		asm {
			pei menu+2
			pei menu
			lda #5 // Dispose
			jsl __Hierarchic
		}
	}
}