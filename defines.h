#define	kAppleMID  	1
#define	kFileMID    	2
#define	kEditMID     	3
#define kNetworkMID		4

#define	kUndoItem   	250
#define	kCutItem    	251
#define	kCopyItem    	252
#define	kPasteItem    	253
#define	kClearItem 	254
#define	kCloseItem	255 

#define	AppleBase	0x1000
#define FileBase	0x2000
#define EditBase	0x3000
#define NetworkBase 0x4000


#define	kAbout		AppleBase+0
#define kPreferences AppleBase+1

#define	kNewItem	FileBase+0
#define	kOpenItem	FileBase+1
#define	kSaveItem	FileBase+2
#define	kPageSetupItem	FileBase+3
#define	kPrintItem	FileBase+4
#define	kQuitItem	FileBase+5

#define kSelectAllItem EditBase+0

#define kConnectItem NetworkBase+0
#define kDisconnectItem NetworkBase+1
#define kNetworkStatusItem NetworkBase+2

#define	kStartStopID  	1
#define	kMenuBarID	1


#define kIndexWindow 0x0100
#define kTextWindow	0x0200
#define kURLWindow 0x0300
#define kPrefsWindow 0x0400
#define kNetworkWindow 0x0500
#define kAboutWindow 0x0600

#define kGopherURL kURLWindow+1
#define kGopherIndex kIndexWindow+1
#define kGopherText kTextWindow+1

#define kAboutButton kAboutWindow+1
#define kAboutIcon kAboutWindow+2
#define kAboutText kAboutWindow+3


#define kIconText 1
#define kIconBinary 2
#define kIconDocument 3
#define kIconSource 4
#define kIconFolder 5
#define kIconFolderOpen 6
