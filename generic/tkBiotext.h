#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <limits.h>
#include <stddef.h>
#include "tcl.h"
#include "tk.h"

#ifndef DLLEXPORT
#define DLLEXPORT __declspec(dllexport)
#endif

/*
 * Tcl/Tk 9.0 compatibility: Tk 8.x's tk.h defined the Tk_Offset(type,field)
 * macro as a thin wrapper around the standard offsetof(); Tk 9's tk.h drops
 * the macro entirely and expects callers to use offsetof() directly. All of
 * this file's Tk_OptionSpec tables use Tk_Offset(Biotext, ...), so supply
 * the macro ourselves when the Tk headers no longer do -- this is a no-op
 * under Tk 8.x, where Tk_Offset is already defined by tk.h. */
#ifndef Tk_Offset
#define Tk_Offset(type, field) ((int) offsetof(type, field))
#endif

#ifdef USE_XFT
#include <X11/Xft/Xft.h>
#endif

#ifdef MAC_TCL
#include <Xatom.h>
#else
#include <X11/Xatom.h>
#endif /* MAC_TCL */

/* 
 * Taken from TkTable/generic/tkTable.h
 */

#if (TCL_MAJOR_VERSION == 8) && (TCL_MINOR_VERSION == 0) /* Tcl8.0 stuff */
#define Tcl_GetString(objPtr)	Tcl_GetStringFromObj(objPtr, (int *)NULL)
#endif
/* NOTE: this Tcl_GetString() shim only applies to Tcl 8.0 (dead in
 * practice, since this codebase targets 8.6+/9.x); real Tcl 8.4+ and
 * Tcl 9.x both supply their own Tcl_GetString(). No Tcl_Size change is
 * needed here since it is never actually compiled against those. */

/*
 * Tcl 9.0 compatibility shim.
 *
 * Tcl 9 replaces "int" with the new "Tcl_Size" type (a signed, pointer-width
 * integer) as the length/index/count type in a number of core APIs, in
 * order to remove Tcl's old 32-bit string/list-length limit. Tcl 8.x has no
 * such type, so we define it ourselves when building against Tcl 8.x -- this
 * lets a single source file build unmodified against both Tcl 8.6 and Tcl 9.x
 * stubs. Tcl 9 itself already provides Tcl_Size (and TCL_SIZE_MAX /
 * TCL_SIZE_MODIFIER) in tcl.h, so this block is a no-op there.
 *
 * See: https://wiki.tcl-lang.org/page/Tcl+9+functions+using+Tcl_Size
 */
#if TCL_MAJOR_VERSION < 9
/* TEA >= 3.13 already passes -DTcl_Size=int when building against Tcl 8:
 * "typedef int Tcl_Size;" would then expand to "typedef int int;". */
#ifndef Tcl_Size
typedef int Tcl_Size;
#endif
#ifndef TCL_SIZE_MAX
#define TCL_SIZE_MAX INT_MAX
#endif
#ifndef TCL_SIZE_MODIFIER
#define TCL_SIZE_MODIFIER ""
#endif
#endif /* TCL_MAJOR_VERSION < 9 */

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
  XftColor XftCfg;
  XftColor XftCbg;
#endif
  XColor *Cfg;
  XColor *Cbg;
  Tk_3DBorder fgBorder;
  Tk_3DBorder bgBorder;
  GC gc;
  /* holds tags coordinates */
  int nbCoords;
  int nbCoordsAlloc;
  Tcl_Obj *coords;
} Tag;

typedef struct {
    /* widget name */
    char *widgetName;
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
  XColor *XfgColor;
  XColor *XbgColor;
  /* grid alignment */
  int setGrid;
  /* highlight width */
  int highlightWidth;
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
#ifdef WIN32
  /* Windows font built from FontFamily/FontSize/FontWeight by
   * BiotextSetFont (tkBioWin.c). Kept apart from tkfont, which is
   * owned by the "-font" option and freed by Tk_FreeConfigOptions. */
  Tk_Font winFont;
#endif

  Tcl_Obj *tkfontPtr;
  /* Specification of the font */
  char *FontXft;
  char *FontName;
  char *FontXlfd;
    char *FontFamily;
  int FontSize;
    char *FontWeight;
  /* Metrics of the font */
  int lineHeight;
  int ascent;
  int descent;
  int charWidth;
  int charHeight;
  /* Commands associated with x and y scrollbars */
  char *xscrollCmd;
  char *yscrollCmd;
  /* Size of the widget, in pixel */
  int width, height;
  /* Width of 3-D border around whole widget. */
  char *borderWidthPtr;	
  int borderWidth;	
  int selBorderWidth;
  /* size in characters */
  int widthChar, heightChar;
    int fullCols, fullRows;
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
  Tcl_Obj *listTags;
  int nbTags;
  int nbTagsAlloc;
  /* Selection */
  int selFirstX, selLastX;
  int selFirstY, selLastY;
  /* Cursor index, row and col */
  int currentC;
  int currentR;

  /* Cut and Paste */
  int nbSeqsCache;
  int lgSeqsCache;
  int *SeqGrpCache;
  char **SeqMatCache;
  int **MapSeqCache;
  Tag *TagsCache;
  Tcl_Obj *listTagsCache;
  int nbTagsCache;
  
  /* Groups */
  Tcl_Obj *GroupList;
  int nbrGrps;
  int *SeqGrp;
} Biotext;

/*
 * Forward declarations for procedures defined later 
 * in this file:
 */
#ifndef NO_PROTOTYPE
static void DumpCache(Biotext *bioPtr, char *title);
static void DumpMapping(Biotext *bioPtr, char *title);
void DumpSeqs(Biotext *bioPtr, char *title);
void DumpCoordsTag(Biotext *bioPtr, char *titre);
void DumpNullChar(Biotext *bioPtr);
int GetIndexOfTag(register Biotext *bioPtr, char *tagName, int mapOrtag);
void BiotextRebuildTagTable(Biotext *bioPtr);
void BiotextDrawAlignment(register Biotext *bioPtr, Drawable d);

void Biotext_AssignTagColors(Biotext *bioPtr, int iTag);

#ifdef USE_XFT
void XColor2XftColor(XColor *xcol, XftColor *xftcol);
#endif

void Biotext_AssignColorOfTag(Tag *tag, Tk_Font tkfont, Tk_Window tkwin, Display *display);

void Biotext_AssignMappingColors(Biotext *bioPtr);

static int BiotextInsertGapsInTags(register Biotext *bioPtr, char *index, int count);
static int BiotextGetIndex(register Biotext *bioPtr, char *str, int *row_p, int *col_p);
#define BiotextGetIndexObj(bioPtr, objPtr, rowPtr, colPtr) \
  BiotextGetIndex(bioPtr, Tcl_GetString(objPtr), rowPtr, colPtr)

static void BiotextUpdateXScrollbar(Biotext *bioPtr);
static void BiotextUpdateYScrollbar(Biotext *bioPtr);
static void BiotextVisibleXRange(Biotext *bioPtr,
			  double *firstPtr,
			  double *lastPtr);
static void BiotextVisibleYRange(Biotext *bioPtr,
			  double *firstPtr,
			  double *lastPtr);
static int BiotextObjCmd(ClientData clientData,
		  Tcl_Interp *interp,
		  int objc,
		  Tcl_Obj * const objv[]);
static void BiotextDeletedProc(ClientData clientData);
static int  BiotextConfigure(Tcl_Interp *interp,
		      Biotext *bioPtr,
		      int objc,
		      Tcl_Obj * const objv[]);
static void BiotextCmdDeletedProc(void *memPtr);
static void BiotextDisplay(ClientData clientData);
static void BiotextObjEventProc(ClientData clientData,
			 XEvent *eventPtr);
static int  BiotextWidgetObjCmd(ClientData clientData,
			 Tcl_Interp *, 
			 int objc, 
			 Tcl_Obj * const objv[]);
static int  BiotextSequencesCmd(Biotext *bioPtr,
			 Tcl_Interp *, 
			 int objc, 
			 Tcl_Obj * const objv[]);
static int BiotextAddSeqsCmd(Biotext *bioPtr,
		      Tcl_Interp *, 
		      int objc, 
		      Tcl_Obj * const objv[]);
static int *MapOneSequence(Biotext *bioPtr, char *seq);
static void MapSequences(Biotext *bioPtr);
static int BiotextMappingCmd(Biotext *bioPtr,
		      Tcl_Interp *, 
		      int objc, 
		      Tcl_Obj * const objv[]);
static int BiotextTagCmd(Biotext *bioPtr,
		  Tcl_Interp *, 
		  int objc, 
		  Tcl_Obj * const objv[]);
static int BiotextTagAddSubCmd(Biotext *bioPtr,
		  Tcl_Interp *, 
		  int objc, 
		  Tcl_Obj * const objv[]);
static int BiotextTagConfigSubCmd(Biotext *bioPtr,
		  Tcl_Interp *, 
		  int objc, 
		  Tcl_Obj * const objv[]);
static int BiotextTagRemoveSubCmd(Biotext *bioPtr,
		  Tcl_Interp *, 
		  int objc, 
		  Tcl_Obj * const objv[]);
static int BiotextTagStateSubCmd(Biotext *bioPtr,
		  Tcl_Interp *, 
		  int objc, 
		  Tcl_Obj * const objv[]);
static int BiotextTagRangesSubCmd(Biotext *bioPtr,
		  Tcl_Interp *, 
		  int objc, 
		  Tcl_Obj * const objv[]);
static int BiotextTagNamesSubCmd(Biotext *bioPtr,
		  Tcl_Interp *, 
		  int objc, 
		  Tcl_Obj * const objv[]);
static int BiotextTagDeleteSubCmd(Biotext *bioPtr,
		  Tcl_Interp *, 
		  int objc, 
		  Tcl_Obj * const objv[]);
static int BiotextTagRaiseSubCmd(Biotext *bioPtr,
		  Tcl_Interp *, 
		  int objc, 
		  Tcl_Obj * const objv[]);
static int BiotextMapCmd(Biotext *bioPtr,
		  Tcl_Interp *, 
		  int objc, 
		  Tcl_Obj * const objv[]);
static int BiotextXViewCmd(register Biotext *bioPtr,
		    Tcl_Interp *, 
		    int objc, 
		    Tcl_Obj * const objv[]);
static int BiotextYViewCmd(register Biotext *bioPtr,
		    Tcl_Interp *, 
		    int objc, 
		    Tcl_Obj * const objv[]);
static int BiotextSelectionCmd(Biotext *bioPtr,
			Tcl_Interp *interp,
			int objc,
			Tcl_Obj * const objv[]);
static void Biotext_Coords2Pixels(Biotext *bioPtr,
			   int r,
			   int c,
			   int *x,
			   int *y,
			   int flag);
static void Biotext_Pixels2Coords(Biotext *bioPtr,
			   int x,
			   int y,
			   int *r,
			   int *c);
static int Biotext_IsVisible(register Biotext *bioPtr, 
		      int xc, 
		      int yc, 
		      int flag);
//static void BiotextFocusProc(Biotext *bioPtr, int gotFocus);
static void BiotextConfigCursor(void *clientData);
static void BiotextFocusProc(Biotext *bioPtr, int gotFocus);
static void BiotextEventuallyRedraw(Biotext *bioPtr);
static void BiotextCursorEvent(ClientData clientData);
static int BiotextInsertCmd(Biotext *bioPtr,
		      Tcl_Interp *interp,
		      int objc,
		      Tcl_Obj * const objv[]);
static int  BiotextInsertCols(register Biotext *bioPtr, 
			      int index, 
			      int count,
			      int row);
static int  BiotextInsertRows(register Biotext *bioPtr, 
		       int index, 
		       int count);
static int  BiotextDeleteCmd(Biotext *bioPtr,
		      Tcl_Interp *interp,
		      int objc,
		      Tcl_Obj * const objv[]);
static int  BiotextDeleteColsGaps(register Biotext *bioPtr); 
static int  BiotextDeleteCols(register Biotext *bioPtr, 
		       int index);
static int  BiotextDeleteRows(register Biotext *bioPtr, 
		       int index);

static int  BiotextGroupCmd(Biotext *bioPtr,
		     Tcl_Interp *interp,
		     int objc,
		     Tcl_Obj * const objv[]);
static int  BiotextUngroupCmd(Biotext *bioPtr,
		       Tcl_Interp *interp,
		       int objc,
		       Tcl_Obj * const objv[]);
static int  BiotextPushCmd(Biotext *bioPtr,
		    Tcl_Interp *interp,
		    int objc,
		    Tcl_Obj * const objv[]);
static int  BiotextFindNextGap(register Biotext *bioPtr,
			int iseq,
			       int icol,
			       int direction);
static int  BiotextFindNextGapOfGroup(register Biotext *bioPtr,
			       int iseq,
			       int iGrp,
				      int icol,
				      int direction);
static void BiotextPushInSeq(register Biotext *bioPtr,
		      int iseq,
		      int icol,
		      int imax);
static int  BiotextIndexCmd(Biotext *bioPtr,
		     Tcl_Interp *interp,
		     int objc,
		     Tcl_Obj * const objv[]);
static int  BiotextDeleteGroup(Biotext *bioPtr,
			int nm);
static void ChangeBiotextXView(register Biotext *bioPtr, int index);
static void ChangeBiotextYView(register Biotext *bioPtr, int index);
static void BiotextComputeGeometry(register Biotext *bioPtr,
				   int width,
				   int height,
				   int request);
static void BiotextWorldChanged(ClientData instanceData);
int BiotextSetFont(register Biotext *bioPtr);
int BiotextSetFont2(register Biotext *bioPtr);
static int BiotextInitBuffer(register Biotext *bioPtr,
		      int nseq);
static int BiotextCopySeqsToBuffer(register Biotext *bioPtr,
			    int nseq,
			    Tcl_Obj **OIdx);
static int BiotextAddColsToBufferSeqs(register Biotext *bioPtr, 
			       int ncols);
static int BiotextSeeCmd(register Biotext *bioPtr,
		   Tcl_Interp *interp,
		   int objc,
		   Tcl_Obj * const objv[]);
static int  BiotextCutCmd(register Biotext *bioPtr,
		   Tcl_Interp *interp,
		   int objc,
		   Tcl_Obj * const objv[]);
static int  BiotextCursorCmd(register Biotext *bioPtr,
		      Tcl_Interp *interp,
		      int objc,
		      Tcl_Obj * const objv[]);
static int  BiotextCopyCmd(register Biotext *bioPtr,
		    Tcl_Interp *interp,
		    int objc,
		    Tcl_Obj * const objv[]);
static int  BiotextPasteCmd(register Biotext *bioPtr,
		     Tcl_Interp *interp,
		     int objc,
			    Tcl_Obj * const objv[]);
static int FontIsMonospace(Display *display, int screen, const char *family, int pixelsize);
static int FontAlreadySeen(char *name, char **list, int count);static 
int  BiotextFontCmd(register Biotext *bioPtr,
		    Tcl_Interp *interp,
		    int objc,
		    Tcl_Obj * const objv[]);
static int BiotextCharsCmd(register Biotext *bioPtr,
		    Tcl_Interp *interp,
		    int objc,
		    Tcl_Obj * const objv[]);
static int  BiotextOutputCmd(register Biotext *bioPtr,
		      Tcl_Interp *interp,
		      int objc,
		      Tcl_Obj * const objv[]);
static void  BiotextCleanCmd(register Biotext *bioPtr,
		      Tcl_Interp *interp,
		      int objc,
		      Tcl_Obj * const objv[]);
static int  BiotextDelcharsCmd(register Biotext *bioPtr,
			Tcl_Interp *interp,
			int objc,
			Tcl_Obj * const objv[]);
static int  BiotextLockCmd(register Biotext *bioPtr,
		    Tcl_Interp *interp,
		    int objc,
		    Tcl_Obj * const objv[]);
void Tk_DrawImageString(Display *display,
		   Drawable d,
		   Tk_Font tkfont,
		   GC gc,
		   int x, int y, 
		   char *string,
		   int length);
#endif /* prototypes */

void BiotextFontInfo(Biotext *bioPtr);


int DLLEXPORT Biotext_Init(Tcl_Interp *interp);


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
#define DEF_BIOTEXT_FONT "Courier 14"
#else
#define DEF_BIOTEXT_FONT "TkFixedFont"
//#define DEF_BIOTEXT_FONT "-adobe-courier-medium-r-normal--11-0-75-75-m-0-iso8859-1"
#define DEF_BIOTEXT_FONT3 "-misc-fixed-medium-r-normal--10-100-75-75-c-60-iso8859-1"
#define DEF_BIOTEXT_FONT2 "-misc-fixed-medium-r-normal--11-0-75-75-c-0-iso8859-1"
#endif






