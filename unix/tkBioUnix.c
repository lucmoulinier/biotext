#define NO_PROTOTYPE 1
#include "../generic/tkBiotext.h"
#include "tkUnixInt.h"
#include "tkFont.h"
#include <X11/Xft/Xft.h>

#define GetFontAttributes(tkfont) \
	((const TkFontAttributes *) &((TkFont *) (tkfont))->fa)


typedef struct UnixFtFace {
  XftFont *ftFont;
  XftFont *ft0Font;
  FcPattern *source;
  FcCharSet *charset;
  double angle;
} UnixFtFace;

typedef struct UnixFtFont {
  TkFont *font;
  UnixFtFace *faces;
  int nfaces;
  FcFontSet *fontset;
  FcPattern *pattern;
  
  Display *display;
  int screen;
  XftDraw *ftDraw;
  XftColor color;
} UnixFtFont;



#define FONTMAP_SHIFT		10
#define FONTMAP_PAGES		(1 << (sizeof(Tcl_UniChar)*8 - FONTMAP_SHIFT))
#define FONTMAP_BITSPERPAGE	(1 << FONTMAP_SHIFT)

typedef struct FontFamily {
  struct FontFamily *nextPtr;	/* Next in list of all known font families. */
  int refCount;		/* How many SubFonts are referring to this
			 * FontFamily. When the refCount drops to
			 * zero, this FontFamily may be freed. */
  /*
   * Key.
   */
  
  Tk_Uid foundry;		/* Foundry key for this FontFamily. */
  Tk_Uid faceName;		/* Face name key for this FontFamily. */
  Tcl_Encoding encoding;	/* Encoding key for this FontFamily. */
  
  /*
   * Derived properties.
   */
  
  int isTwoByteFont;		/* 1 if this is a double-byte font, 0
				 * otherwise. */
  char *fontMap[FONTMAP_PAGES];
				/* Two-level sparse table used to determine
				 * quickly if the specified character exists.
				 * As characters are encountered, more pages
				 * in this table are dynamically alloced. The
				 * contents of each page is a bitmask
				 * consisting of FONTMAP_BITSPERPAGE bits,
				 * representing whether this font can be used
				 * to display the given character at the
				 * corresponding bit position. The high bits
				 * of the character are used to pick which
				 * page of the table is used. */
} FontFamily;

typedef struct SubFont {
  char **fontMap;		/* Pointer to font map from the FontFamily,
				 * cached here to save a dereference. */
  XFontStruct *fontStructPtr;	/* The specific screen font that will be used
				 * when displaying/measuring chars belonging
				 * to the FontFamily. */
  FontFamily *familyPtr;	/* The FontFamily for this SubFont. */
} SubFont;

#define SUBFONT_SPACE		3
#define BASE_CHARS		256

typedef struct UnixFont {
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
  SubFont controlSubFont;	/* Font to use to display control-character
				 * expansions. */
  
  Display *display;		/* Display that owns font. */
  int pixelSize;		/* Original pixel size used when font was
				 * constructed. */
  TkXLFDAttributes xa;	/* Additional attributes that specify the
			 * preferred foundry and encoding to use when
			 * constructing additional SubFonts. */
  int widths[BASE_CHARS];	/* Widths of first 256 chars in the base font,
				 * for handling common case. */
  int underlinePos;		/* Offset from baseline to origin of underline
				 * bar (used when drawing underlined font)
				 * (pixels). */
  int barHeight;		/* Height of underline or overstrike bar (used
				 * when drawing underlined or strikeout font)
				 * (pixels). */
} UnixFont;

typedef struct EncodingAlias {
  const char *realName;	/* The real name of the encoding to load if
			   the provided name matched the pattern. */
  const char *aliasPattern;	/* Pattern for eoding name, of the form that
				 * is acceptable to Tcl_StringMatch. */
} EncodingAlias;

typedef struct FontAttributes {
  TkFontAttributes fa;
  TkXLFDAttributes xa;
} FontAttributes;

typedef struct ThreadSpecificData {
    FontFamily *fontFamilyList; /* The list of font families that are
				 * currently loaded. As screen fonts are
				 * loaded, this list grows to hold information
				 * about what characters exist in each font
				 * family. */
  FontFamily controlFamily;	/* FontFamily used to handle control character
				 * expansions. The encoding of this FontFamily
				 * converts UTF-8 to backslashed escape
				 * sequences. */
} ThreadSpecificData;


void
XColor2XftColor(XColor *xcol,
		XftColor *xftcol)
{
  xftcol->pixel       = xcol[0].pixel;
  xftcol->color.red   = xcol[0].red;
  xftcol->color.green = xcol[0].green;
  xftcol->color.blue  = xcol[0].blue;
  xftcol->color.alpha = 0xffff;

  return;
}

void 
Biotext_AssignMappingColors(register Biotext *bioPtr) 
{
  int i;
  XColor *Cfg, *Cbg;

  for (i=0; i<bioPtr->nbMapping;i++) {
    Cfg = bioPtr->Mapping[i].Cfg;
    Cbg = bioPtr->Mapping[i].Cbg;

    bioPtr->Mapping[i].XftCfg.pixel = Cfg[0].pixel;
    bioPtr->Mapping[i].XftCfg.color.red = Cfg[0].red;
    bioPtr->Mapping[i].XftCfg.color.green = Cfg[0].green;
    bioPtr->Mapping[i].XftCfg.color.blue = Cfg[0].blue;
    bioPtr->Mapping[i].XftCfg.color.alpha = 0xffff;
    
    bioPtr->Mapping[i].XftCbg.pixel = Cbg[0].pixel;
    bioPtr->Mapping[i].XftCbg.color.red = Cbg[0].red;
    bioPtr->Mapping[i].XftCbg.color.green = Cbg[0].green;
    bioPtr->Mapping[i].XftCbg.color.blue = Cbg[0].blue;
    bioPtr->Mapping[i].XftCbg.color.alpha = 0xffff;
  }
  
  return;
}

void 
Biotext_AssignTagColors(register Biotext *bioPtr,
			 int k) 
{
  XColor *Cfg, *Cbg;

  Cfg = bioPtr->Tags[k].Cfg;
  Cbg = bioPtr->Tags[k].Cbg;

  bioPtr->Tags[k].XftCfg.pixel = bioPtr->Tags[k].fg;
  bioPtr->Tags[k].XftCfg.color.red = Cfg[0].red;
  bioPtr->Tags[k].XftCfg.color.green = Cfg[0].green;
  bioPtr->Tags[k].XftCfg.color.blue = Cfg[0].blue;
  bioPtr->Tags[k].XftCfg.color.alpha = 0xffff;
    
  bioPtr->Tags[k].XftCbg.pixel = bioPtr->Tags[k].fg;
  bioPtr->Tags[k].XftCbg.color.red = Cbg[0].red;
  bioPtr->Tags[k].XftCbg.color.green = Cbg[0].green;
  bioPtr->Tags[k].XftCbg.color.blue = Cbg[0].blue;
  bioPtr->Tags[k].XftCbg.color.alpha = 0xffff;
  
  return;
}

 
/*--------------------------------------------------------------
 *
 * BiotextSetFont --
 *
 *      This routine sets the font according to 
 *      FontFamily, FontSize and FontWeight stored in
 *      the Biotext structure.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None
 *
 *--------------------------------------------------------------
 */
#define DEBUG_SETFONT 0

int 
BiotextSetFont(register Biotext *bioPtr) 
{
#if DEBUG_SETFONT
  fprintf(stderr,"\n== BiotextSetFont ==========\n");
  fprintf(stderr,"widget %s\n", bioPtr->widgetName);
  fprintf(stderr,"In: family %s size %d weight %s\n", bioPtr->FontFamily, bioPtr->FontSize, bioPtr->FontWeight);
  fflush(stderr);
#endif
    Tcl_Interp *interp = bioPtr->interp;

    /*--------------------------------------------------*/
    /* 1. Build fontconfig pattern (keep the previous    */
    /*    font around until a new one has successfully   */
    /*    loaded -- see bug-fix note below).              */
    /*--------------------------------------------------*/
    /* Bug fix: this function used to close bioPtr->xftFont
     * and immediately NULL it out before even attempting to
     * load the replacement font. Every error path below
     * (pattern creation failure, no matching font, font open
     * failure) then returned TCL_ERROR with bioPtr->xftFont
     * left at NULL -- leaving the widget with no valid font
     * at all. The next BiotextDrawAlignment() call passes
     * this NULL font straight to XftDrawString8(), which
     * crashes. Fix: keep the OLD font alive in a local
     * variable and only close it once the new font has
     * loaded successfully; on any error, bioPtr->xftFont
     * keeps pointing at the still-valid old font. */
    XftFont *oldFont = bioPtr->xftFont;

    /*--------------------------------------------------*/
    /* 2. Build fontconfig pattern                      */
    /*--------------------------------------------------*/
    FcPattern *pattern = FcPatternCreate();
    if (!pattern) {
        Tcl_SetResult(interp, "fontconfig: failed to create pattern", TCL_STATIC);
        return TCL_ERROR;
    }

    FcPatternAddString (pattern, FC_FAMILY,     (FcChar8 *)bioPtr->FontFamily);
    FcPatternAddDouble (pattern, FC_PIXEL_SIZE, (double)bioPtr->FontSize);
    FcPatternAddInteger(pattern, FC_SPACING,    FC_MONO);

    if (bioPtr->FontWeight && strcmp(bioPtr->FontWeight, "bold") == 0) {
        FcPatternAddInteger(pattern, FC_WEIGHT, FC_WEIGHT_BOLD);
    } else {
        /* Bug fix: previously, the "normal" (default) case added no
         * FC_WEIGHT constraint to the pattern at all, leaving the
         * actual resolved weight entirely up to fontconfig's default
         * substitution. In practice this is usually FC_WEIGHT_REGULAR,
         * but it is not guaranteed on every system/font-config setup
         * (e.g. if the requested family has no Regular face installed,
         * or under a fontconfig configuration that biases substitution
         * towards a heavier default weight) -- this is the most likely
         * explanation for the widget occasionally coming up bold by
         * default on some systems, even though the code never
         * requested "bold". Being explicit here makes the "normal"
         * case just as deterministic as the "bold" one, independent of
         * whatever a given system's fontconfig defaults happen to be. */
        FcPatternAddInteger(pattern, FC_WEIGHT, FC_WEIGHT_REGULAR);
    }

    FcConfigSubstitute(NULL, pattern, FcMatchPattern);
    FcDefaultSubstitute(pattern);

    /*--------------------------------------------------*/
    /* 3. Match best font                               */
    /*--------------------------------------------------*/
    FcResult   fcresult;
    FcPattern *match = XftFontMatch(bioPtr->display,
                                    Tk_ScreenNumber(bioPtr->tkwin),
                                    pattern,
                                    &fcresult);
    FcPatternDestroy(pattern);     /* always safe here — match is a new pattern */
    
    if (!match) {
        /* Bug fix: keep the previously loaded font (bioPtr->xftFont
         * is untouched so far -- we haven't overwritten it yet). */
        Tcl_SetResult(interp, "xft: no matching font found", TCL_STATIC);
        return TCL_ERROR;
    }

    /*--------------------------------------------------*/
    /* 4. Open font  (match ownership transfers to Xft) */
    /*--------------------------------------------------*/
    {
        XftFont *newFont = XftFontOpenPattern(bioPtr->display, match);
        if (!newFont) {
            /* ownership was NOT transferred on failure */
            FcPatternDestroy(match);
            /* Bug fix: bioPtr->xftFont still holds the old, valid font
             * here -- leave it alone rather than leaving it NULL. */
            Tcl_SetResult(interp, "xft: failed to open font", TCL_STATIC);
            return TCL_ERROR;
        }
        /* Do NOT FcPatternDestroy(match) here — Xft owns it now */

        /* New font loaded successfully: NOW it is safe to close the
         * old one and switch bioPtr->xftFont over to the new one. */
        if (oldFont != NULL) {
            XftFontClose(bioPtr->display, oldFont);
        }
        bioPtr->xftFont = newFont;
    }

    /*--------------------------------------------------*/
    /* 5. Retrieve actual metrics from opened font      */
    /*--------------------------------------------------*/
    FcPattern *fp = bioPtr->xftFont->pattern;

    /* Fontconfig may substitute a different size — read back the real one */
    double pixelsize;
    if (FcPatternGetDouble(fp, FC_PIXEL_SIZE, 0, &pixelsize) == FcResultMatch)
        bioPtr->FontSize = (int)(pixelsize + 0.5);   /* round, don't truncate */

    /* Read back the actual family chosen */
    FcChar8 *actual_family = NULL;
    if (FcPatternGetString(fp, FC_FAMILY, 0, &actual_family) == FcResultMatch
            && actual_family != NULL) {
        ckfree(bioPtr->FontFamily);
        bioPtr->FontFamily = ckalloc(strlen((char *)actual_family) + 1);
        strcpy(bioPtr->FontFamily, (char *)actual_family);
    }

    /* Bug fix: bioPtr->FontWeight was only ever written to reflect what
     * was REQUESTED (in BiotextFontCmd), never corrected to reflect what
     * fontconfig actually resolved -- unlike bioPtr->FontFamily just
     * above, which IS kept in sync with the real match. If the
     * requested weight ("bold", say) has no matching face for this
     * family/size and fontconfig substitutes a different weight, the
     * glyphs actually drawn no longer match what bioPtr->FontWeight (and
     * so ".b font weight") reports, with no way for a caller to notice.
     * Read the resolved FC_WEIGHT back from the matched pattern and
     * re-derive "bold"/"normal" from it, the same way the family is
     * re-derived above. Fontconfig's weight scale is on-going/ordinal,
     * not binary, so anything at or past the midpoint between
     * FC_WEIGHT_REGULAR and FC_WEIGHT_BOLD is reported as "bold" --
     * consistent with the only two weights this widget ever requests. */
    int actual_weight;
    if (FcPatternGetInteger(fp, FC_WEIGHT, 0, &actual_weight) == FcResultMatch) {
        const char *resolved = (actual_weight >= (FC_WEIGHT_REGULAR + FC_WEIGHT_BOLD) / 2)
            ? "bold" : "normal";
        if (bioPtr->FontWeight == NULL || strcmp(bioPtr->FontWeight, resolved) != 0) {
            if (bioPtr->FontWeight != NULL) {
                ckfree(bioPtr->FontWeight);
            }
            bioPtr->FontWeight = ckalloc(strlen(resolved) + 1);
            strcpy(bioPtr->FontWeight, resolved);
        }
    }

    bioPtr->charWidth  = bioPtr->xftFont->max_advance_width;
    bioPtr->charHeight = bioPtr->xftFont->height;
    bioPtr->ascent     = bioPtr->xftFont->ascent;
    bioPtr->descent    = bioPtr->xftFont->descent;
    bioPtr->lineHeight = bioPtr->charHeight;

#if DEBUG_SETFONT
    fprintf(stderr, "BioSetFont: family=%s size=%d "
                    "charWidth=%d charHeight=%d ascent=%d descent=%d\n",
            bioPtr->FontFamily, bioPtr->FontSize,
            bioPtr->charWidth, bioPtr->charHeight,
            bioPtr->ascent, bioPtr->descent);
    fflush(stderr);
#endif

    /*--------------------------------------------------*/
    /* 6. Request geometry update                       */
    /*--------------------------------------------------*/
    int bdi = bioPtr->borderWidth + bioPtr->highlightWidth;
    int reqW = bioPtr->charWidth  * bioPtr->widthChar  + 2 * bdi;
    int reqH = bioPtr->charHeight * bioPtr->heightChar + 2 * bdi;

    Tk_GeometryRequest(bioPtr->tkwin, reqW, reqH);

    return TCL_OK;
}


void 
BiotextDrawAlignment(register Biotext *bioPtr, 
		     Drawable d) 
{
  Tcl_Interp *interp = bioPtr->interp;
  Display *display = bioPtr->display;
  Tk_Window tkwin = bioPtr->tkwin;
  Visual *vis;
  Colormap cmap;
  XftFont *ftFont = NULL;
  XftDraw *ftDraw = NULL;

  int hc, wc, LinesPerPage, ResPerPage;
  int startXC, startYC, finYC, finXC;
  int startXCOri, startYCOri, finXCOri, finYCOri;
  int yP, xP, i, j, k;
  int depth;
  int ascent, k0, lg, xP0;
  unsigned long fg;
  char *Str = NULL;

  /* shortcuts for Biotext structure variables */
  int **MapSeq = bioPtr->MapSeq;
  char **Seq = bioPtr->SeqMat;
  Tag *Map = bioPtr->Mapping;
  Tag *Tag = bioPtr->Tags;
  
  vis = Tk_GetVisual(interp, tkwin, "default", &depth, &cmap);    
  ftDraw = XftDrawCreate(display, d, vis, cmap);
  ftFont = bioPtr->xftFont;

  int bdi;

  bdi = bioPtr->borderWidth + bioPtr->highlightWidth;
  
  LinesPerPage = MIN(bioPtr->nbSeqs, bioPtr->heightChar);
  ResPerPage   = MIN(bioPtr->lgSeqs, bioPtr->widthChar);
  
  startXC = bioPtr->leftIndex;
  finXC   = MIN(startXC+ResPerPage,bioPtr->lgSeqs);
  startYC = bioPtr->topIndex;
  finYC   = MIN(startYC+LinesPerPage, bioPtr->nbSeqs);
  
  Str     = (char *)ckalloc((ResPerPage+1)*sizeof(char));
  wc      = bioPtr->charWidth;
  hc       = bioPtr->lineHeight;
  ascent  = bioPtr->ascent;
  
  /* 
   * Start Drawing line by line
   */
  yP = bdi + ascent;
  
  if (! (bioPtr->flags & MAP_RESIDUES)) {
    /*
     * No mapping applied
     * Use defaults foreground and background colors
     */
      int totalX;
      int nVisible = finXC - startXC;   /* real number of visible columns:
                                          * can be < ResPerPage when scrolled
                                          * to the last screenful of a
                                          * sequence whose length isn't an
                                          * exact multiple of the visible
                                          * width (see bug-fix note below) */

      /* Bug fix: this used to unconditionally use ResPerPage (the
       * *maximum* number of columns that fit on screen) instead of the
       * actual number of visible columns (finXC-startXC) for the
       * background rect width, the strncpy() length and the
       * XftDrawString8() glyph count. When scrolled to the last
       * screenful of a sequence (startXC + ResPerPage > lgSeqs), this
       * drew a background rectangle that overran the real end of the
       * sequence, and asked Xft to draw ResPerPage glyphs from Str even
       * though strncpy() had zero-padded the tail of Str past the
       * sequence's real length -- rendering stray NUL/"missing glyph"
       * boxes past the end of each row. No memory-safety issue (Str is
       * still sized ResPerPage+1 and strncpy() never reads past Seq[i]'s
       * own NUL terminator), just visibly wrong rendering. */
      totalX = wc * nVisible;
      for (i=startYC; i<finYC; i++) {      
	  xP = bdi;
      XftDrawRect(ftDraw, &(bioPtr->bgColor), xP, yP-ascent, totalX, hc);
      strncpy(Str, Seq[i]+startXC, nVisible);
      Str[nVisible] = '\0';
      XftDrawString8(ftDraw, &(bioPtr->fgColor), ftFont, xP, yP, (XftChar8 *)Str, nVisible);
      yP += hc;
    }	
  } else {
    /* 
     * 'map' residues ON
     */
#if 0
      fprintf(stderr, "\nwidget %s\n", bioPtr->widgetName);
      fprintf(stderr, "LinesPerPage %d ResPerPage %d\n", LinesPerPage, ResPerPage);
      fprintf(stderr, "startXC %d finXC %d\n", startXC, finXC);
      fprintf(stderr, "startYC %d finYC %d\n", startYC, finYC);
      fprintf(stderr, " MapSeq startYC , startXC %d\n", MapSeq[startYC][startXC]);
      fflush(stderr);
#endif      
    yP = bdi + ascent;
    for (i=startYC; i<finYC; i++) {
	xP0 = xP = bdi;
      lg = 0;
      Str[0] = '\0';
      k0 = MapSeq[i][startXC];
      fg = Map[k0].fg;
      for (j=startXC;j<finXC;j++) {
	k = MapSeq[i][j];
	XftDrawRect(ftDraw, &Map[k].XftCbg, xP, yP-ascent, wc, hc);
	
	if (Map[k].fg != fg) {
	  XftDrawString8(ftDraw, &Map[k0].XftCfg, ftFont, xP0, yP, (XftChar8 *) Str, lg);
	  lg = 0;
	  xP0 = xP;
	  k0 = k;
	  fg = Map[k].fg;
	  Str[0] = '\0';
	}
	
	Str[lg] = Seq[i][j];
	lg++;
	
	xP += wc;
      }
      if (lg != 0) {
	//XftDrawRect(ftDraw, &Map[k].XftCbg, xP, yP-ascent, wc, hc);
	XftDrawString8(ftDraw, &Map[k0].XftCfg, ftFont, xP0, yP, (XftChar8 *) Str, lg);
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
    Tcl_Obj *tagObj, *valObj;
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
	/* Bug fix: this used to do
	 *   tmpObj = Tcl_NewListObj(0, NULL);
	 *   tmpObj = Tcl_DuplicateObj(eltObj[i]);
	 * The first line allocated a fresh empty Tcl_Obj (refCount 0)
	 * that was immediately discarded by the second assignment
	 * without ever being used or freed -- a leak on every tagged
	 * row, on every single redraw. The duplicate itself was also
	 * never released (no matching Tcl_DecrRefCount), leaking a
	 * second object per row per redraw on top of that. Since we
	 * only ever READ from it here (Tcl_ListObjLength /
	 * Tcl_ListObjIndex), there is no need to duplicate eltObj[i]
	 * at all -- it is already a valid, live Tcl_Obj owned by
	 * Tag[iTag].coords for the duration of this draw call. */
	if (Tcl_ListObjLength(interp, eltObj[i], &len) != TCL_OK) {
	  //fprintf(stderr, "\nCan't get length of obj %d\n",i);
	  //fflush(stderr);
	  continue;
	}
	if (len <= 0) {
	  continue;
	}
	
	for(j=0; j<len; j+=2) {
	  Tcl_ListObjIndex(interp, eltObj[i], j, &valObj);
	  Tcl_GetIntFromObj(interp, valObj, &c1);
	  Tcl_ListObjIndex(interp, eltObj[i], j+1, &valObj);
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
	  
	  yP = bioPtr->borderWidth + ascent + (startYC - startYCOri) * hc;
	  xP = wc * (startXC - startXCOri);
	  for (ic=startYC;ic<=finYC;ic++) {
	    XftDrawRect(ftDraw, &Tag[iTag].XftCbg, xP, yP-ascent, wc*(finXC-startXC+1), hc);
	    XftDrawString8(ftDraw, &Tag[iTag].XftCfg, ftFont, xP, yP, (XftChar8 *) Seq[ic]+startXC, (finXC-startXC+1));
	  }
	  yP += hc;
	}
      }
    }
  }
  
  XftDrawDestroy(ftDraw);
  ckfree((char *)Str);
  
  return;
}


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
/*
void
Tk_DrawImageString(Display *display,
		   Drawable d,
		   Tk_Font tkfont,
		   GC gc,
		   int x, int y, 
		   char *string,
		   int length)
{
    XDrawImageString(display, d, gc, x, y, string, length);

  return;
}
*/

