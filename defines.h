#define	kAppleMID  	1
#define	kFileMID    	2
#define	kEditMID     	3
#define kTextMID		4
#define kNetworkMID		5
#define kBookmarkMID	6
#define kWindowMID		7

#define kH_TextMID		8



#define	kUndoItem   	250
#define	kCutItem    	251
#define	kCopyItem    	252
#define	kPasteItem    	253
#define	kClearItem 	254
#define	kCloseItem	255 

#define	AppleBase	0x1000
#define FileBase	0x2000
#define EditBase	0x3000
#define TextBase	0x4000
#define NetworkBase 0x5000
#define BookmarkBase 0x6000
#define WindowBase	0x7000
#define H_TextBase	0x8000


#define	kAbout		AppleBase+0
#define kPreferences AppleBase+1

#define	kNewItem	FileBase+0
#define	kOpenItem	FileBase+1
#define	kSaveItem	FileBase+2
#define	kPageSetupItem	FileBase+3
#define	kPrintItem	FileBase+4
#define	kQuitItem	FileBase+5

#define kSelectAllItem EditBase+0

#define kWrapTextItem TextBase+0
#define kDeDotItem TextBase+1
#define kTab4Item TextBase+2
#define kTab8Item TextBase+3
#define kEncoding1Item TextBase+4
#define kEncoding2Item TextBase+5
#define kBinsciiItem TextBase+6

#define kH_TabItem H_TextBase+0
#define kH_Tab4Item H_TextBase+1
#define kH_Tab8Item H_TextBase+2
#define kH_EncodingItem H_TextBase+3
#define kH_Encoding1Item H_TextBase+4
#define kH_Encoding2Item H_TextBase+5



#define kConnectItem NetworkBase+0
#define kDisconnectItem NetworkBase+1
#define kNetworkStatusItem NetworkBase+2

#define kAddBookmarkItem BookmarkBase+0

#define	kStartStopID  	1
#define	kMenuBarID	1


#define kIndexWindow 0x0100
#define kTextWindow	0x0200
#define kURLWindow 0x0300
#define kPrefsWindow 0x0400
#define kNetworkWindow 0x0500
#define kAboutWindow 0x0600
#define kSearchWindow 0x0700


#define kSearchText kSearchWindow+1
#define kSearchLineEdit kSearchWindow+2


#define kGopherURL kURLWindow+1
#define kGopherIndex kIndexWindow+1
#define kGopherText kTextWindow+1

#define kAboutButton kAboutWindow+1
#define kAboutIcon kAboutWindow+2
#define kAboutText kAboutWindow+3


#define kNetworkStatusLeft kNetworkWindow+1
#define kNetworkStatusRight kNetworkWindow+2
#define kNetworkIPLeft kNetworkWindow+3
#define kNetworkIPRight kNetworkWindow+4
#define kNetworkConnect kNetworkWindow+5
#define kNetworkDisconnect kNetworkWindow+6

#define kIconText 1
#define kIconBinary 2
#define kIconDocument 3
#define kIconSource 4
#define kIconFolder 5
#define kIconFolderOpen 6
#define kIconSearch 7
