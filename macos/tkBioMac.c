#include "tkBiotext.h"
//#include "tkUnixInt.h"
#include "tkFont.h"



static const char *const encodingList[] = {
  "iso8859-1", "jis0208", "jis0212", NULL
};

/*
 * The following structure represents a font family. It is assumed that all
 * screen fonts constructed from the same "font family" share certain
 * properties; all screen fonts with the same "font family" point to a shared
 * instance of this structure. The most important shared property is the
 * character existence metrics, used to determine if a screen font can display
 * a given Unicode character.
 *
 * Under Unix, there are three attributes that uniquely identify a "font
 * family": the foundry, face name, and charset.
 */

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

/*
 * The following structure encapsulates an individual screen font. A font
 * object is made up of however many SubFonts are necessary to display a
 * stream of multilingual characters.
 */

typedef struct SubFont {
  char **fontMap;		/* Pointer to font map from the FontFamily,
				 * cached here to save a dereference. */
  XFontStruct *fontStructPtr;	/* The specific screen font that will be used
				 * when displaying/measuring chars belonging
				 * to the FontFamily. */
  FontFamily *familyPtr;	/* The FontFamily for this SubFont. */
} SubFont;

/*
 * The following structure represents Unix's implementation of a font object.
 */

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

/*
 * The following structure and definition is used to keep track of the
 * alternative names for various encodings. Asking for an encoding that
 * matches one of the alias patterns will result in actually getting the
 * encoding by its real name.
 */

typedef struct EncodingAlias {
  const char *realName;	/* The real name of the encoding to load if
			 * the provided name matched the pattern. */
  const char *aliasPattern;	/* Pattern for encoding name, of the form that
				 * is acceptable to Tcl_StringMatch. */
} EncodingAlias;

/*
 * Just some utility structures used for passing around values in helper
 * functions.
 */

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
static Tcl_ThreadDataKey dataKey;

/*
 * The set of builtin encoding alises to convert the XLFD names for the
 * encodings into the names expected by the Tcl encoding package.
 */

static EncodingAlias encodingAliases[] = {
  {"gb2312-raw",	"gb2312*"},
  {"big5",		"big5*"},
  {"cns11643-1",	"cns11643*-1"},
  {"cns11643-1",	"cns11643*.1-0"},
  {"cns11643-2",	"cns11643*-2"},
  {"cns11643-2",	"cns11643*.2-0"},
  {"jis0201",		"jisx0201*"},
  {"jis0201",		"jisx0202*"},
  {"jis0208",		"jisc6226*"},
  {"jis0208",		"jisx0208*"},
  {"jis0212",		"jisx0212*"},
  {"tis620",		"tis620*"},
  {"ksc5601",		"ksc5601*"},
  {"dingbats",	"*dingbats"},
#ifdef WORDS_BIGENDIAN
  {"unicode",		"iso10646-1"},
#else
  /*
   * ucs-2be is needed if native order isn't BE.
   */
  {"ucs-2be",		"iso10646-1"},
#endif
  {NULL,		NULL}
};



void BiotextDrawAlignment(register Biotext *BiotextPtr, Drawable d) 
{
  int hc, wc, LinesPerPage, ResPerPage, startXC;
  int startYC, yP, xP, i, j, k;
  Display *display = BiotextPtr->display;
  Tk_Window tkwin = BiotextPtr->tkwin;
  Colormap cmap;
  int depth;

  LinesPerPage = MIN(BiotextPtr->nbSeqs,BiotextPtr->heightChar);
  ResPerPage   = MIN(BiotextPtr->lgSeqs,BiotextPtr->widthChar);
  Tag *TM = BiotextPtr->TagMap;
  char **SM = BiotextPtr->SeqMat;
  int x, **ST = BiotextPtr->SeqT;

  wc = BiotextPtr->charWidth;
  hc = BiotextPtr->charHeight;
  startXC = BiotextPtr->leftIndex;
  startYC = BiotextPtr->topIndex;

  xP = BiotextPtr->borderWidth+BiotextPtr->inset;
  for (j=startXC;j<startXC+ResPerPage;j++) {
    yP = BiotextPtr->borderWidth + BiotextPtr->inset + BiotextPtr->ascent;
    for (i=startYC;i<startYC+LinesPerPage;i++) {
      k = ST[i][j];
      XDrawImageString(display, d, TM[k].gc, xP, yP, SM[i]+j, 1);
      yP += hc;
    }
    xP += wc;
  } 

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



/*
  Hello !

  I need help by a guru in ATSU/text drawing !
  I'm porting an app written in C to Mac.
  This app draws aligned protein sequences, i.e. a succession of letters (20 letters + '.'). Each letter has a foreground and a background color (F white fg, red bg). The font is ALWAYS the same for all characters and is a fixed font like Courier. My app consists in drawing a matrix of colored characters in a single font, the characters being drawn depend on the position of the scrollbars.
  The inner drawing loop is someething like :
  y=y0;
  for (j=startY;j<startY+linesPerPage;j++) {
  x=x0;
  for (i=startX;i<startX+charsPerLine;i++) {
  c  = T[j][i];
  fg = FgColor[c];
  bg = BgColor[c];
  <DrawText(context, layout, c, fg, bg, x, y,font)>
  }
  }

  The routines to draw text are :
  Unix :
  -with Xft enabled : 
  XftDrawRect, XftDrawString8
  -without Xft :
  XDrawImageString
  Windows :
  - set drawable context SetBkColor(hdc,bg) , SetTextColor(hdc, fg)
  - print text : TextOut(hdc,x, y, string, length)

  For sake of compatibility, I must use the ATSUI framework.

  Can someone explain me how to achieve this ?
  - where goes the foreground/background color ? in the style or layout ?
  - should I use ATSUDrawText ?
  - where should I initialise the layout ?

  Thanks for your help ! Please unserstand that I'm primarily a biologist, and that I can't learn all drawing API !! ;-)

  Luc 

  Je cherche un gourou en ATSUI / dessin de texte !
  Je porte mon appli sur Mac.
  Cette appli dessine un alignement de séquence de protéines (je suis biologiste à la base), c.a.d une succession de lettres (20 + '.'). Chaque lettre a un foreground et un background (p.e. F fg=whte, bg=red). La fonte est TOUJOURS la même pour tous les caractères, et est toujours une fonte 'fixed' (comme Courier). Mon appli consiste donc à dessiner une matrices de caractères 
  colorés dans une fonte unique, les caractères à écrire dépendants de la position des scrollbars.
  Ma boucle interne de dessin ressemble à :
  y=y0;
  for (j=startY;j<startY+linesPerPage;j++) {
  x=x0;
  for (i=startX;i<startX+charsPerLine;i++) {
  c  = T[j][i];
  fg = FgColor[c];
  bg = BgColor[c];
  <DrawText(context, layout, c, fg, bg, x, y,font)>
  }
  }

  Les fonctions de bas niveau utilisées pour dessiner :
  Unix :
  -avec Xft : 
  XftDrawRect, XftDrawString8
  -sansXft :
  XDrawImageString
  Windows :
  - set drawable context SetBkColor(hdc,bg) , SetTextColor(hdc, fg)
  - print text : TextOut(hdc,x, y, string, length)

  Pour des questions de compatibilité, je suis obligé d'utiliser ATSU !!

  Quelqu'un peut-il me guider pour écrire ma boucle ?
  - où va les couleurs foreground et background ? dans un style ou un layout ?
  - dois-je utiliser ATSUDrawText ?
  - où dois-je initialiser le layout ?

  Merci de m'aider !! S'il vous plait, comprenez que je suis biologiste à la base, et que je ne peux pas apprendre toutes API !!

  Merci !!!

  Luc 



*/






