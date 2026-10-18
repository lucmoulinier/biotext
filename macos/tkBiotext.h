#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "tcl.h"
#include "tk.h"

#ifdef USE_XFT
#include <X11/Xft/Xft.h>
#endif

#ifdef MAC_TCL
# include <Xatom.h>
#else
# include <X11/Xatom.h>
#endif /* MAC_TCL */

/* 
 * Take all from TkTable/generic/tkTable.h
 * Hope this works ...
 */

#if (TCL_MAJOR_VERSION == 8) && (TCL_MINOR_VERSION == 0) /* Tcl8.0 stuff */
#define Tcl_GetString(objPtr)	Tcl_GetStringFromObj(objPtr, (int *)NULL)
#endif

#if (TCL_MAJOR_VERSION > 8) || ((TCL_MAJOR_VERSION == 8) && (TCL_MINOR_VERSION >= 4))
#   define HAVE_TCL84
#endif

/*
 * Tcl/Tk 8.4 introduced better CONST-ness in the APIs, but we use CONST84 in
 * some cases for compatibility with earlier Tcl headers to prevent warnings.
 */
#ifndef CONST84
#  define CONST84
#endif

/* This EXTERN declaration is needed for Tcl < 8.0.3 */
#ifndef EXTERN
# ifdef __cplusplus
#  define EXTERN extern "C"
# else
#  define EXTERN extern
# endif
#endif

#ifdef TCL_STORAGE_CLASS
# undef TCL_STORAGE_CLASS
#endif
#ifdef BUILD_Tktable
# define TCL_STORAGE_CLASS DLLEXPORT
#else
# define TCL_STORAGE_CLASS DLLIMPORT
#endif

#ifdef WIN32
#   define WIN32_LEAN_AND_MEAN
#   include <windows.h>
#   include <tkWinInt.h>
#   undef WIN32_LEAN_AND_MEAN
/* VC++ has an entry point called DllMain instead of DllEntryPoint */
#   if defined(_MSC_VER)
#	define DllEntryPoint DllMain
#   endif
#endif

#if defined(WIN32) || defined(MAC_TCL) || defined(MAC_OSX_TK)
/* XSync call defined in the internals for some reason */
#   ifndef XSync
#	define XSync(display, bool) {display->request++;}
#   endif
#endif /* defn of XSync */

#ifndef NORMAL_BG
#   ifdef WIN32
#	define NORMAL_BG	"SystemButtonFace"
#	define ACTIVE_BG	NORMAL_BG
#	define SELECT_BG	"SystemHighlight"
#	define SELECT_FG	"SystemHighlightText"
#	define DISABLED		"SystemDisabledText"
#	define HIGHLIGHT	"SystemWindowFrame"
#	define DEF_TABLE_FONT	"{MS Sans Serif} 8"
#   elif defined(MAC_TCL) || defined(MAC_OSX_TK)
#	define NORMAL_BG	"systemWindowBody"
#	define ACTIVE_BG	"#ececec"
#	define SELECT_BG	"systemHighlight"
#	define SELECT_FG	"systemHighlightText"
#	define DISABLED		"#a3a3a3"
#	define HIGHLIGHT	"Black"
#	define DEF_TABLE_FONT	"Helvetica 12"
#   else
#	define NORMAL_BG	"#d9d9d9"
#	define ACTIVE_BG	"#fcfcfc"
#	define SELECT_BG	"#c3c3c3"
#	define SELECT_FG	"Black"
#	define DISABLED		"#a3a3a3"
#	define HIGHLIGHT	"Black"
#	define DEF_TABLE_FONT	"Helvetica -12"
#   endif
#endif /* NORMAL_BG */

#define MAX(A,B)	(((A)>(B))?(A):(B))
#define MIN(A,B)	(((A)>(B))?(B):(A))

/*
 * A data structure of the following type is kept 
 * for each Biotext widget
 * managed by this file:
 */

typedef struct {
  char smap;
  char *tagName;
  unsigned long fg;
  unsigned long bg;

#ifdef USE_XFT
  XftColor Cfg;
  XftColor Cbg;
#else
  XColor Cfg;
  XColor Cbg;
#endif
  //Tk_3DBorder fg;
  Tk_3DBorder bgBorder;
  GC gc;
} Tag;

typedef struct {
  /* Class of the widget */
  char *className;
  /* Window that embodies the Biotext. NULL means
   * window has been deleted but widget record
   * hasn't been cleaned up yet. */
  Tk_Window tkwin; 
  /* X's token for the window's display. */
  Display *display;		
  /* scren associated */
  Screen *screen;
  /* Interpreter associated with widget. */
  Tcl_Interp *interp;
  /* Token for Biotext's widget command. */
  Tcl_Command widgetCmd;	
  /* Token representing the configuration
   * specifications. */
  Tk_OptionTable optionTable;	
    
  /*
   * Information used when displaying widget:
   */
  /* State : editable or not */
  int state;
  /* Width of 3-D border around whole widget. */
  Tcl_Obj *borderWidthPtr;	
  int borderWidth;	
  /* Color Ptr for background */
  Tcl_Obj *bgBorderPtr;
  Tk_3DBorder bgBorder;
  /* Color Ptr for foreground */
  Tcl_Obj *fgBorderPtr;
  Tk_3DBorder fgBorder;
#ifdef USE_XFT
  XftColor fgColor;
  XftColor bgColor;
#else
  XColor fgColor;
  XColor bgColor;
#endif
  /* relief of the border */
  Tcl_Obj *reliefPtr;
  int relief;
  /* Graphics context for copying from
   * off-screen pixmap onto screen. */
  GC gc;			
  /* Non-zero means double-buffer redisplay with
   * pixmap; zero means draw straight onto the
   * display. */
  int doubleBuffer;	
  /* Non-zero means a call to BiotextDisplay has
   * already been scheduled. */
  int updatePending;		
  /* Font system */
  int xft;
  /* Font to use */
  Tk_Font tkfont; 
  Font font;
#ifdef USE_XFT
  /* font pour xft */
  XftFont *xftFont;
#endif

  Tcl_Obj *tkfontPtr;
  /* Specification of the font */
  char *FontXft;
  char *FontName;
  char *FontXlfd;
  char FontFamily[100];
  int FontSize;
  char FontWeight[100];
  /* Metrics of the font */
  int ascent;
  int descent;
  int charWidth;
  int charHeight;
  /* Commands associated with x and y scrollbars */
  char *xscrollCmd;
  char *yscrollCmd;
  /* Size of the widget, in pixel */
  int width, height;
  /* size in characters */
  int widthChar, heightChar;
  Tcl_Obj *widthCharPtr, *heightCharPtr;
  /* index of the left most and top most visible
   * character */
  int leftIndex;
  int topIndex;
  int inset;
  int flags;
  XColor *cursorColor;
  int insertOffTime;
  /* Number of milliseconds cursor should spend in "on" state for each blink. */
  int insertOnTime;  
  /* timer token for the cursor blinking */
  Tcl_TimerToken cursorTimer; 
  /* Timer handler used to blink cursor on and off */
  Tcl_TimerToken insertBlinkHandler; 
 
  /* 
   * Data storage and control
   */
  /* alignment matrix */
  char **SeqMat;
  /* mapping matrix */
  int  **MapSeq;
  /* tags matrix */
  int **TagSeq;
  /* seq attributes */
  int nbSeqs;
  int lgSeqs;
  int lgAlloc;
  /* sequence locking : prevent erasing/insertion */
  int Lock;
  /* mapping */
  int nbMapping;
  /* holds mapping info */
  Tag *Mapping;
  /* tagging */
  Tag *Tags;
  int nbTags;
  /* Selection */
  int selFirstX, selLastX;
  int selFirstY, selLastY;
  /* Cursor index, row and col */
  int currentC;
  int currentR;

  /* Cut and Paste */
  int nbrSeqsCache;
  int lgSeqsCache;
  int *SeqGrpCache;
  char **SeqMatCache;
  int **MapSeqCache;
  int **TagSeqCache;

  /* Groups */
  int nbrGrps;
  int *SeqGrp;
  Tcl_Obj *GroupList;
} Biotext;


/*
 * Forward declarations for procedures defined later 
 * in this file:
 */

void BiotextDrawAlignment(register Biotext *BiotextPtr, Drawable d);
void Biotext_AssignColors(Biotext *BiotextPtr, int flag);

extern int BiotextGetIndex(register Biotext *BiotextPtr, char *str, int *row_p, int *col_p);
#define BiotextGetIndexObj(BiotextPtr, objPtr, rowPtr, colPtr) \
        BiotextGetIndex(BiotextPtr, Tcl_GetString(objPtr), rowPtr, colPtr)

void BiotextUpdateXScrollbar(Biotext *BiotextPtr);
void BiotextUpdateYScrollbar(Biotext *BiotextPtr);
void BiotextVisibleXRange(Biotext *BiotextPtr,
			  double *firstPtr,
			  double *lastPtr);
void BiotextVisibleYRange(Biotext *BiotextPtr,
			  double *firstPtr,
			  double *lastPtr);
int BiotextObjCmd(ClientData clientData,
		  Tcl_Interp *interp,
		  int objc,
		  Tcl_Obj * const objv[]);
void BiotextDeletedProc(ClientData clientData);
int  BiotextConfigure(Tcl_Interp *interp,
		      Biotext *BiotextPtr,
		      int objc,
		      Tcl_Obj * const objv[]);
void BiotextDestroy(char *memPtr);
void BiotextDisplay(ClientData clientData);
void BiotextObjEventProc(ClientData clientData,
			 XEvent *eventPtr);
int  BiotextWidgetObjCmd(ClientData clientData,
			 Tcl_Interp *, 
			 int objc, 
			 Tcl_Obj * const objv[]);
int  BiotextSequencesCmd(Biotext *BiotextPtr,
			 Tcl_Interp *, 
			 int objc, 
			 Tcl_Obj * const objv[]);
int BiotextAddSeqsCmd(Biotext *BiotextPtr,
		      Tcl_Interp *, 
		      int objc, 
		      Tcl_Obj * const objv[]);
int BiotextMappingCmd(Biotext *BiotextPtr,
		      Tcl_Interp *, 
		      int objc, 
		      Tcl_Obj * const objv[]);
int BiotextTagCmd(Biotext *BiotextPtr,
		  Tcl_Interp *, 
		  int objc, 
		  Tcl_Obj * const objv[]);
int BiotextMapCmd(Biotext *BiotextPtr,
		  Tcl_Interp *, 
		  int objc, 
		  Tcl_Obj * const objv[]);
int BiotextXViewCmd(register Biotext *BiotextPtr,
		    Tcl_Interp *, 
		    int objc, 
		    Tcl_Obj * const objv[]);
int BiotextYViewCmd(register Biotext *BiotextPtr,
		    Tcl_Interp *, 
		    int objc, 
		    Tcl_Obj * const objv[]);
int BiotextSelectionCmd(Biotext *BiotextPtr,
			Tcl_Interp *interp,
			int objc,
			Tcl_Obj * const objv[]);
void Biotext_Coords2Pixels(Biotext *BiotextPtr,
			   int r,
			   int c,
			   int *x,
			   int *y,
			   int flag);
void Biotext_Pixels2Coords(Biotext *BiotextPtr,
			   int x,
			   int y,
			   int *r,
			   int *c);
int Biotext_IsVisible(register Biotext *BiotextPtr, 
		      int xc, 
		      int yc, 
		      int flag);
void BiotextFocusProc(Biotext *BiotextPtr,
		      int gotFocus);
void BiotextConfigCursor(register Biotext *BiotextPtr);
void BiotextEventuallyRedraw(Biotext *BiotextPtr);
void BiotextCursorEvent(ClientData clientData);
int  BiotextInsertCmd(Biotext *BiotextPtr,
		      Tcl_Interp *interp,
		      int objc,
		      Tcl_Obj * const objv[]);
int  BiotextInsertCols(register Biotext *BiotextPtr, 
		       int index, 
		       int count);
int  BiotextInsertRows(register Biotext *BiotextPtr, 
		       int index, 
		       int count);
int  BiotextDeleteCmd(Biotext *BiotextPtr,
		      Tcl_Interp *interp,
		      int objc,
		      Tcl_Obj * const objv[]);
int  BiotextDeleteColsGaps(register Biotext *BiotextPtr); 
int  BiotextDeleteCols(register Biotext *BiotextPtr, 
		       int index);
int  BiotextDeleteRows(register Biotext *BiotextPtr, 
		       int index);

int  BiotextGroupCmd(Biotext *BiotextPtr,
		     Tcl_Interp *interp,
		     int objc,
		     Tcl_Obj * const objv[]);
int  BiotextUngroupCmd(Biotext *BiotextPtr,
		       Tcl_Interp *interp,
		       int objc,
		       Tcl_Obj * const objv[]);
int  BiotextPushCmd(Biotext *BiotextPtr,
		    Tcl_Interp *interp,
		    int objc,
		    Tcl_Obj * const objv[]);
int  BiotextFindNextGap(register Biotext *BiotextPtr,
			int iseq,
			int icol);
int  BiotextFindNextGapOfGroup(register Biotext *BiotextPtr,
			       int iseq,
			       int iGrp,
			       int icol);
void BiotextPushInSeq(register Biotext *BiotextPtr,
		      int iseq,
		      int icol,
		      int imax);
int  BiotextIndexCmd(Biotext *BiotextPtr,
		     Tcl_Interp *interp,
		     int objc,
		     Tcl_Obj * const objv[]);
int  BiotextDeleteGroup(Biotext *BiotextPtr,
			int nm);
void BiotextComputeGeometry(register Biotext *BiotextPtr);
void BiotextWorldChanged(ClientData instanceData);
int BiotextSetFont(register Biotext *BiotextPtr);
int BiotextInitBuffer(register Biotext *BiotextPtr,
		      int nseq);
int BiotextCopySeqsToBuffer(register Biotext *BiotextPtr,
			    int nseq,
			    Tcl_Obj **OIdx);
int BiotextAddColsToBufferSeqs(register Biotext *BiotextPtr, 
			       int ncols);
int BiotextSeeCmd(register Biotext *BiotextPtr,
		   Tcl_Interp *interp,
		   int objc,
		   Tcl_Obj * const objv[]);
int  BiotextCutCmd(register Biotext *BiotextPtr,
		   Tcl_Interp *interp,
		   int objc,
		   Tcl_Obj * const objv[]);
int  BiotextCursorCmd(register Biotext *BiotextPtr,
		      Tcl_Interp *interp,
		      int objc,
		      Tcl_Obj * const objv[]);
int  BiotextCopyCmd(register Biotext *BiotextPtr,
		    Tcl_Interp *interp,
		    int objc,
		    Tcl_Obj * const objv[]);
int  BiotextPasteCmd(register Biotext *BiotextPtr,
		     Tcl_Interp *interp,
		     int objc,
		     Tcl_Obj * const objv[]);
int  BiotextFontCmd(register Biotext *BiotextPtr,
		    Tcl_Interp *interp,
		    int objc,
		    Tcl_Obj * const objv[]);
int BiotextCharsCmd(register Biotext *BiotextPtr,
		    Tcl_Interp *interp,
		    int objc,
		    Tcl_Obj * const objv[]);
int  BiotextOutputCmd(register Biotext *BiotextPtr,
		      Tcl_Interp *interp,
		      int objc,
		      Tcl_Obj * const objv[]);
void  BiotextCleanCmd(register Biotext *BiotextPtr,
		      Tcl_Interp *interp,
		      int objc,
		      Tcl_Obj * const objv[]);
int  BiotextDelcharsCmd(register Biotext *BiotextPtr,
			Tcl_Interp *interp,
			int objc,
			Tcl_Obj * const objv[]);
int  BiotextLockCmd(register Biotext *BiotextPtr,
		    Tcl_Interp *interp,
		    int objc,
		    Tcl_Obj * const objv[]);
void 
Tk_DrawImageString(Display *display,
		   Drawable d,
		   Tk_Font tkfont,
		   GC gc,
		   int x, int y, 
		   char *string,
		   int length);
void BiotextFontInfo(Biotext *BiotextPtr);
void DumpSeqs(Biotext *BiotextPtr);
void RemoveTaggedPositionsInAlignment(Biotext *BiotextPtr, 
				      int iTag);
EXTERN int Biotext_Init(Tcl_Interp *interp);


/* Definition des flags */
#define REDRAW_PENDING          1
#define BORDER_NEEDED           2
#define CURSOR_ON               4
#define GOT_FOCUS               8
#define UPDATE_SCROLLBAR        0x10
#define GOT_SELECTION           0x20
#define BIOTEXT_DELETED         0x40
#define MAP_RESIDUES            0x80
#define TAG_ALIGNMENT           0x100

static const char *const stateStrings[] = {
  "disabled", "normal", NULL
};

enum state {
  STATE_DISABLED, STATE_NORMAL
};

/*
 * default values 
 */
#define INDEX_BUFSIZE 32
#if defined(WIN32)
#define DEF_BIOTEXT_FONT "Courier 10"
#else
#define DEF_BIOTEXT_FONT "-adobe-courier-medium-r-normal--11-0-75-75-m-0-iso8859-1"
#define DEF_BIOTEXT_FONT3 "-misc-fixed-medium-r-normal--10-100-75-75-c-60-iso8859-1"
#define DEF_BIOTEXT_FONT2 "-misc-fixed-medium-r-normal--11-0-75-75-c-0-iso8859-1"
#endif







