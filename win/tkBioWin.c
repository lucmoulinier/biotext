#define NO_PROTOTYPE 1
#include "../generic/tkBiotext.h"
#include <tkFont.h>
#include <tkWinInt.h>

/* Taken from tk8.6.0b1/win/tkWinFont.c */
/*
 * The following structure represents a font family. It is assumed that all
 * screen fonts constructed from the same "font family" share certain
 * properties; all screen fonts with the same "font family" point to a shared
 * instance of this structure. The most important shared property is the
 * character existence metrics, used to determine if a screen font can display
 * a given Unicode character.
 *
 * Under Windows, a "font family" is uniquely identified by its face name.
 */

#define FONTMAP_SHIFT	    10

#define FONTMAP_PAGES	    	(1 << (sizeof(Tcl_UniChar)*8 - FONTMAP_SHIFT))
#define FONTMAP_BITSPERPAGE	(1 << FONTMAP_SHIFT)

void BiotextFontInfo(register Biotext *bioPtr);

typedef struct FontFamily {
    struct FontFamily *nextPtr;	/* Next in list of all known font families. */
    int refCount;		/* How many SubFonts are referring to this
				 * FontFamily. When the refCount drops to
				 * zero, this FontFamily may be freed. */
    /*
     * Key.
     */

    Tk_Uid faceName;		/* Face name key for this FontFamily. */

    /*
     * Derived properties.
     */

    Tcl_Encoding encoding;	/* Encoding for this font family. */
    int isSymbolFont;		/* Non-zero if this is a symbol font. */
    int isWideFont;		/* 1 if this is a double-byte font, 0
				 * otherwise. */
    BOOL (WINAPI *textOutProc)(HDC hdc, int x, int y, TCHAR *str, int len);
				/* The procedure to use to draw text after it
				 * has been converted from UTF-8 to the
				 * encoding of this font. */
    BOOL (WINAPI *getTextExtentPoint32Proc)(HDC, TCHAR *, int, LPSIZE);
				/* The procedure to use to measure text after
				 * it has been converted from UTF-8 to the
				 * encoding of this font. */

    char *fontMap[FONTMAP_PAGES];
				/* Two-level sparse table used to determine
				 * quickly if the specified character exists.
				 * As characters are encountered, more pages
				 * in this table are dynamically added. The
				 * contents of each page is a bitmask
				 * consisting of FONTMAP_BITSPERPAGE bits,
				 * representing whether this font can be used
				 * to display the given character at the
				 * corresponding bit position. The high bits
				 * of the character are used to pick which
				 * page of the table is used. */

    /*
     * Cached Truetype font info.
     */

    int segCount;		/* The length of the following arrays. */
    USHORT *startCount;		/* Truetype information about the font, */
    USHORT *endCount;		/* indicating which characters this font can
				 * display (malloced). The format of this
				 * information is (relatively) compact, but
				 * would take longer to search than indexing
				 * into the fontMap[][] table. */
} FontFamily;

/*
 * The following structure encapsulates an individual screen font. A font
 * object is made up of however many SubFonts are necessary to display a
 * stream of multilingual characters.
 */

typedef struct SubFont {
    char **fontMap;		/* Pointer to font map from the FontFamily,
				 * cached here to save a dereference. */
    HFONT hFont0;		/* The specific screen font that will be used
				 * when displaying/measuring chars belonging
				 * to the FontFamily. */
    FontFamily *familyPtr;	/* The FontFamily for this SubFont. */
    HFONT hFontAngled;
    double angle;
} SubFont;

/*
 * The following structure represents Windows' implementation of a font
 * object.
 */

#define SUBFONT_SPACE		3
#define BASE_CHARS		128

typedef struct WinFont {
    TkFont font;		/* Stuff used by generic font package. Must be
				 * first in structure. */
    SubFont staticSubFonts[SUBFONT_SPACE];
				/* Builtin space for a limited number of
				 * SubFonts. */
    int numSubFonts;		/* Length of following array. */
    SubFont *subFontArray;	/* Array of SubFonts that have been loaded in
				 * order to draw/measure all the characters
				 * encountered by this font so far. All fonts
				 * start off with one SubFont initialized by
				 * AllocFont() from the original set of font
				 * attributes. Usually points to
				 * staticSubFonts, but may point to malloced
				 * space if there are lots of SubFonts. */
    HWND hwnd;			/* Toplevel window of application that owns
				 * this font, used for getting HDC for
				 * offscreen measurements. */
    int pixelSize;		/* Original pixel size used when font was
				 * constructed. */
    int widths[BASE_CHARS];	/* Widths of first 128 chars in the base font,
				 * for handling common case. The base font is
				 * always used to draw characters between
				 * 0x0000 and 0x007f. */
} WinFont;

/*
 *--------------------------------------------------------------
 *
 * Tk_DrawImageString --
 *	Draw a character with a given color, and
 *      draws the underlying background of the bbox 
 *      containing those characters with a given 
 *      color.
 *
 * Results:
 *      None. 
 *
 * Side effects:
 *      None.      
 *
 *--------------------------------------------------------------
 */
void
Tk_DrawImageString(Display *display,
		   Drawable d,
		   Tk_Font tkfont,
		   GC gc,
		   int x, int y, 
		   char *string,
		   int length) 
{    
  if (d == None) {
    return;
  }
    
  HDC hdc;
  HFONT oldFont;
  LPTSTR lpchText;
  WinFont *fontPtr;
  SubFont *lastSubFontPtr;
  TkWinDCState state;

  lpchText = string;

  fontPtr = (WinFont *) tkfont;
  lastSubFontPtr = &(fontPtr->subFontArray[0]);
  hdc = TkWinGetDrawableDC(display, d, &state);
  SetROP2(hdc, SRCCOPY);

  SetBkMode(hdc, OPAQUE);
  SetBkColor(hdc, gc->background);
  SetTextColor(hdc, gc->foreground);

  oldFont = SelectObject(hdc, lastSubFontPtr->hFont0);
  
  TextOut(hdc, x, y, lpchText, 1);

  SelectObject(hdc, oldFont);
  TkWinReleaseDrawableDC(d, hdc, &state);

  return;
}

/*
 * Font actually used for drawing and metrics: the one built from
 * FontFamily/FontSize/FontWeight by BiotextSetFont(), or the "-font"
 * option as a fallback if that font could not be built.
 */
static Tk_Font
BiotextGetWinFont(Biotext *bioPtr)
{
  return (bioPtr->winFont != NULL) ? bioPtr->winFont : bioPtr->tkfont;
}

/*
 *--------------------------------------------------------------
 *
 * BiotextFontInfo --
 *      Gives font information. Only use non-Xft fonts
 *
 * Results:
 *      None. 
 *
 * Side effects:
 *      None.      
 *
 *--------------------------------------------------------------
 */
void
BiotextFontInfo(register Biotext *bioPtr) 
{
  Tk_FontMetrics fm;
  Tk_Font tkfont = BiotextGetWinFont(bioPtr);

  /* Bug fix: BiotextObjCmd (tkBiotext.c) calls BiotextSetFont() --
   * and therefore this function -- while building the widget record,
   * i.e. BEFORE Tk_InitOptions() has filled in the "-font" option.
   * At that point bioPtr->tkfont is still NULL (the record is
   * memset() to 0), and Tk_GetFontMetrics(NULL, ...) dereferences it:
   * immediate crash on every widget creation under Windows
   * (msys2/mingw64). BiotextSetFont() now builds winFont first, so this
   * only happens if that failed; leave the metrics alone then. */
  if (tkfont == NULL) {
    return;
  }

  Tk_GetFontMetrics(tkfont, &fm);
  
  bioPtr->charWidth  = Tk_TextWidth(tkfont, "Z", 1);
  bioPtr->charHeight = fm.linespace;
  bioPtr->ascent     = fm.ascent;
  bioPtr->descent    = fm.descent;
  /* Bug fix: this used to do "bioPtr->FontSize = fm.linespace;",
   * overwriting the user-requested point/pixel size (set via
   * ".biotext font size N", and reported back by tkBiotext.c's
   * "font size" getter) with an unrelated rendering metric
   * (linespace = ascent + descent + leading, typically a few pixels
   * larger than the requested size). Left untouched here so
   * FontSize keeps meaning "the size that was requested", consistent
   * with its use elsewhere and with the Unix/Xft implementation. */
  bioPtr->lineHeight = fm.linespace;
  
  return;
}


int 
BiotextSetFont(register Biotext *bioPtr)
{
  /* Bug fix: this used to only read the "-font" option (tkfont), so
   * ".biotext font family|size|weight", which set FontFamily, FontSize
   * and FontWeight, had no visible effect under Windows. Build the
   * font from those fields instead, like the Unix/Xft version does
   * with fontconfig. FontSize is in pixels there (FC_PIXEL_SIZE), hence
   * the negative size given to Tk here (negative = pixels in Tk). */
  Tcl_Interp *interp = bioPtr->interp;
  Tcl_Obj *descObj;
  Tk_Font newFont;
  const char *weight;

  if (bioPtr->tkwin != NULL && bioPtr->FontFamily != NULL) {
    weight = (bioPtr->FontWeight != NULL && strcmp(bioPtr->FontWeight, "bold") == 0) ? "bold" : "normal";

    /* Built as a Tcl list so that family names with spaces
     * ("Courier New", "Lucida Console") are passed as one element. */
    descObj = Tcl_NewListObj(0, NULL);
    Tcl_IncrRefCount(descObj);
    Tcl_ListObjAppendElement(NULL, descObj, Tcl_NewStringObj(bioPtr->FontFamily, -1));
    Tcl_ListObjAppendElement(NULL, descObj, Tcl_NewIntObj(-bioPtr->FontSize));
    Tcl_ListObjAppendElement(NULL, descObj, Tcl_NewStringObj(weight, -1));
    newFont = Tk_GetFont(interp, bioPtr->tkwin, Tcl_GetString(descObj));
    Tcl_DecrRefCount(descObj);

    if (newFont == NULL) {
      /* Keep the previous font (same policy as the Unix version):
       * Tk_GetFont() has already set the error message in interp. */
      return TCL_ERROR;
    }
    /* Free the old font only once the new one is loaded. */
    if (bioPtr->winFont != NULL) {
      Tk_FreeFont(bioPtr->winFont);
    }
    bioPtr->winFont = newFont;
  }

  if (BiotextGetWinFont(bioPtr) == NULL) {
    return TCL_OK;
  }

  BiotextFontInfo(bioPtr);

  bioPtr->font = Tk_FontId(BiotextGetWinFont(bioPtr));

  return TCL_OK;
}


void 
Biotext_AssignMappingColors(register Biotext *bioPtr) {
  return;
}


void 
Biotext_AssignTagColors(register Biotext *bioPtr,
			int k) {
  return;
}


void 
BiotextDrawAlignment(register Biotext *bioPtr,
		     Drawable d) 
{
  Tcl_Interp *interp = bioPtr->interp;
  int hc, wc, lg, LinesPerPage, ResPerPage, startXC;
  int startYC, finYC, finXC, yP, xP, xP0, i, j, k, k0;
  char *Str = NULL;

  Display *display = bioPtr->display;
  HDC hdc;
  HFONT oldFont;
  WinFont *fontPtr;
  SubFont *lastSubFontPtr;
  TkWinDCState state;
  unsigned long fg, bg;

  /* shortcuts for Biotext structure variables */
  int **MapSeq = bioPtr->MapSeq;
  char **Seq = bioPtr->SeqMat;
  Tag *Map = bioPtr->Mapping;
  Tag *Tag = bioPtr->Tags;
  
  fontPtr = (WinFont *) BiotextGetWinFont(bioPtr);
  lastSubFontPtr = &(fontPtr->subFontArray[0]);
  hdc = TkWinGetDrawableDC(display, d, &state);
  SetROP2(hdc, SRCCOPY);
  SetBkMode(hdc, OPAQUE);
  oldFont = SelectObject(hdc, lastSubFontPtr->hFont0);

  LinesPerPage = MIN(bioPtr->nbSeqs,bioPtr->heightChar);
  ResPerPage   = MIN(bioPtr->lgSeqs,bioPtr->widthChar);

  startXC = bioPtr->leftIndex;
  finXC   = MIN(startXC+ResPerPage,bioPtr->lgSeqs);
  startYC = bioPtr->topIndex;
  finYC   = MIN(startYC+LinesPerPage,bioPtr->nbSeqs);

  Str = (char *)ckalloc((ResPerPage+1)*sizeof(char));
  wc = bioPtr->charWidth;
  hc = bioPtr->charHeight;
    
  yP = bioPtr->borderWidth;
  if (! (bioPtr->flags & MAP_RESIDUES)) {
    /*
     * No mapping applied
     * Use defaults foreground and background colors
     */
    
    /* write line by line*/
    SetBkColor(hdc,Tk_3DBorderColor(bioPtr->bgBorder)->pixel);
    SetTextColor(hdc,Tk_3DBorderColor(bioPtr->fgBorder)->pixel);
    for (i=startYC;i<finYC;i++) {
      /* Bug fix: this used to pass the fixed "ResPerPage" (the
       * *maximum* number of columns that fit on screen) as the
       * character count to TextOut(), reading directly from
       * Seq[i]+startXC with NO intermediate bounded buffer (unlike
       * the Unix/Xft renderer, which copies through strncpy() into a
       * buffer sized Str -- and strncpy() safely stops at the row's
       * own NUL terminator, so it never actually over-reads there).
       * Here, TextOut() has no such protection: it reads exactly
       * ResPerPage characters starting at that pointer regardless of
       * where the row's real data ends. When scrolled to the last
       * screenful of a sequence whose length isn't an exact multiple
       * of the visible width (startXC + ResPerPage > lgSeqs), this
       * read past the end of the row's heap buffer -- a real
       * out-of-bounds read, not just a cosmetic rendering glitch like
       * its Unix counterpart. Use the actual number of visible
       * columns instead. */
      int nVisible = finXC - startXC;
      xP = bioPtr->borderWidth;
      TextOut(hdc, xP, yP, (LPTSTR) Seq[i]+startXC, nVisible);
      yP += hc;
    }
  } else {
    /* 
     * Mapping residues 
     */

    yP = bioPtr->borderWidth;
    for (i=startYC;i<finYC;i++) {
      xP0 = xP = bioPtr->borderWidth;
      lg  = 0;
      Str[0]='\0';
      k0  = MapSeq[i][startXC];
      fg  = Map[k0].fg;
      bg  = Map[k0].bg;
      for (j=startXC;j<finXC;j++) {
	  k = MapSeq[i][j];
	  if (Map[k].fg != fg || Map[k].bg != bg) {
	  SetBkColor(hdc, bg);
	  SetTextColor(hdc, fg);
	  TextOut(hdc, xP0, yP, (LPTSTR) Str, lg);
	  lg=0;
	  xP0 = xP;
	}
	  Str[lg] = Seq[i][j];
	  bg = Map[k].bg;
	fg = Map[k].fg;
	lg++;
	
	xP += wc;
      }
      if (lg > 0) {
	  SetBkColor(hdc,bg);
	SetTextColor(hdc,fg);
	TextOut(hdc, xP0, yP, (LPTSTR) Str, lg);
      }
      yP += hc;
    } 
  }

  /*
   * Tag alignment if necessary
   */
  if ((bioPtr->flags & TAG_ALIGNMENT) && (bioPtr->nbTags > 0)) {
    int iTag, ic, r1, r2, c1, c2;
    Tcl_Size nelt, len;
    int startXCOri, finXCOri, startYCOri, finYCOri;
    Tcl_Obj *tagObj, *valObj, *tmpObj;
    Tcl_Obj **eltObj;
    
    startXCOri = startXC;
    startYCOri = startYC;
    finXCOri   = finXC;
    finYCOri   = finYC;
    
    for(k=0; k<bioPtr->nbTags; k++) {
      Tcl_ListObjIndex(NULL, bioPtr->listTags, k, &tagObj);
      Tcl_GetIntFromObj(NULL, tagObj, &iTag);
      //fprintf(stderr,"k    = %d\n",k);
      //fprintf(stderr,"iTag = %d\n",iTag);
      //fprintf(stderr,"Name = %s\n", Tag[k].tagName);
      //fflush(stderr);
      
      if (Tcl_ListObjGetElements(NULL, Tag[iTag].coords, &nelt, &eltObj) != TCL_OK) {
	//fprintf(stderr,"\nERROR getting tcl_obj array !");
	//fflush(stderr);
	continue;
      } else {
	//fprintf(stderr,"get %d elements\n", nelt);
	//fflush(stderr);
      }
      
      for(i=0; i<nelt; i++) {
	/* Bug fix: same dead-store leak already found and fixed in the
	 * Unix equivalent of this file (tkBioUnix.c) -- the
	 * Tcl_NewListObj(0, NULL) result was immediately discarded and
	 * overwritten by the Tcl_DuplicateObj() call right after,
	 * leaking one Tcl_Obj per tagged row on every single screen
	 * redraw (scroll, resize, edit...). */
	tmpObj = Tcl_DuplicateObj(eltObj[i]);
	if (Tcl_ListObjLength(interp, tmpObj, &len) != TCL_OK) {
	  //fprintf(stderr, "\nCan't get length of obj %d\n",i);
	  //fflush(stderr);
	  continue;
	}
	if (len <= 0) {
	  continue;
	}
	
	for(j=0; j<len; j+=2) {
	  Tcl_ListObjIndex(interp, tmpObj, j, &valObj);
	  Tcl_GetIntFromObj(interp, valObj, &c1);
	  Tcl_ListObjIndex(interp, tmpObj, j+1, &valObj);
	  Tcl_GetIntFromObj(interp, valObj, &c2);

	  r1 = r2 = i;
	  if (c1 > finXCOri || c2 < startXCOri || r1 > finYCOri || r2 < startYCOri) {
	    //fprintf(stderr,"bad %d %d %d %d\n", r1, r2, c1, c2);
	    //fflush(stderr);
	    continue;
	  }
	  
	  //fprintf(stderr, "i= %d\n", i);
	  //fprintf(stderr,"startXCOri %2d c1 %d\n", startXCOri, c1);
	  //fprintf(stderr,"finXCOri   %2d c2 %d\n", finXCOri, c2);
	  
	  startXC = MAX(startXCOri, c1);
	  finXC   = MIN(finXCOri, c2);
	  startYC = MAX(startYCOri, r1);
	  finYC   = MIN(finYCOri, r2);
	  //fprintf(stderr,"startXC %2d finXC %2d\n", startXC, finXC);
	  //fflush(stderr);
	  yP = bioPtr->borderWidth + (startYC - startYCOri) * hc;
	  xP = wc * (startXC - startXCOri);
	  for (ic=startYC;ic<=finYC;ic++) {
	      SetBkColor(hdc, Tag[iTag].bg);
	      SetTextColor(hdc, Tag[iTag].fg);
	      TextOut(hdc, xP, yP, (LPTSTR) Seq[ic]+startXC, (finXC-startXC+1));

	      yP += hc;
	  }
	}
      }
    }
  }

  SelectObject(hdc, oldFont);
  TkWinReleaseDrawableDC(d, hdc, &state);

  ckfree((char *) Str);

  return;
}





