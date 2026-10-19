#include <strings.h>
/*
 * test_biotext_min.h
 *
 * Minimal reconstruction of the parts of "tkBiotext.h" that the
 * functions under test actually touch (SeqMat/MapSeq/SeqGrp handling,
 * copy/cut/paste caches, tags). This lets us compile and exercise the
 * REAL function bodies extracted verbatim from tkBiotext.c against a
 * real Tcl interpreter, without needing Tk/X11 (no widget is created,
 * no window is opened -- everything here is pure data manipulation).
 *
 * Fields not used by the functions under test (fonts, colors used for
 * rendering, GCs, scrolling state, etc.) are intentionally omitted or
 * reduced to a placeholder so the struct stays small and honest about
 * what is actually exercised.
 */
#ifndef TEST_BIOTEXT_MIN_H
#define TEST_BIOTEXT_MIN_H

#include <tcl.h>

/* Tcl 9 uses Tcl_Size for list lengths; Tcl 8.6 has no such type
 * (same shim as generic/tkBiotext.h). */
#if TCL_MAJOR_VERSION < 9 && !defined(Tcl_Size)
typedef int Tcl_Size;
#endif
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define UPDATE_SCROLLBAR 1

typedef struct Mapping {
    char smap;   /* single character mapped by this entry */
    int  itag;   /* associated tag index (unused by our tests) */
} Mapping;

typedef struct Tag {
    char *tagName;
    Tcl_Obj *coords;     /* list of per-row coordinate lists: {start stop start stop ...} */
    int nbCoords;
    unsigned long fg, bg, fgBorder, bgBorder, Cfg, Cbg;
} Tag;

typedef struct Biotext {
    Tcl_Interp *interp;
    char *widgetName;

    /* alignment data */
    int nbSeqs;
    int lgSeqs;
    char **SeqMat;   /* nbSeqs rows, each (lgSeqs+1) bytes, NUL-terminated */
    int  **MapSeq;   /* nbSeqs rows, each lgSeqs ints (only if nbMapping>0) */
    int  *SeqGrp;    /* nbSeqs ints */

    /* residue -> tag mapping table */
    int nbMapping;
    Mapping *Mapping;

    /* tags */
    int nbTags;
    Tag *Tags;
    Tcl_Obj *listTags;

    /* copy/cut/paste cache (one level of undo-ish buffer) */
    int nbSeqsCache;
    int lgSeqsCache;
    char **SeqMatCache;
    int  **MapSeqCache;
    int  *SeqGrpCache;
    int nbTagsCache;
    Tag *TagsCache;
    Tcl_Obj *listTagsCache;

    /* misc used by BiotextGetIndex / BiotextInsertCols / BiotextDeleteCols */
    int currentR, currentC;
    long flags;

    /* pixel<->coord conversion (only used by the "@x,y" index form,
     * which our tests never use, but the field must exist to compile) */
    int inset, charWidth, charHeight, leftIndex, topIndex;
} Biotext;

/* ---- stubs for Tk-side helpers the extracted functions call ---- */

static void BiotextEventuallyRedraw(Biotext *bioPtr) {
    /* no-op: no real Tk window in this test harness */
    (void)bioPtr;
}

static void Biotext_Pixels2Coords(Biotext *bioPtr, int x, int y, int *c, int *r) {
    /* no-op: not exercised by our tests (we never use "@x,y" indices) */
    (void)bioPtr; (void)x; (void)y;
    *c = 0; *r = 0;
}

/* ---- forward declarations of the real functions, extracted verbatim
 * from tkBiotext.c into tkBiotext_extracted_functions.c. Needed because
 * some of them call others that are defined later in that file (e.g.
 * BiotextInsertCols calls GetIndexOfTag and BiotextInsertGapsInTags,
 * both defined after it). If you regenerate the extracted-functions
 * file from a newer tkBiotext.c, keep these in sync (signatures only,
 * copy them from the real function definitions). ---- */
static int BiotextInsertCols(Biotext *bioPtr, int index, int count, int row);
static int BiotextInsertRows(Biotext *bioPtr, int index, int count);
static int BiotextDeleteColsGaps(Biotext *bioPtr);
static int BiotextDeleteCols(Biotext *bioPtr, int index);
static int BiotextDeleteRows(Biotext *bioPtr, int index);
static int BiotextCopyCmd(Biotext *bioPtr, Tcl_Interp *interp, int objc, Tcl_Obj * const objv[]);
static int BiotextCutCmd(Biotext *bioPtr, Tcl_Interp *interp, int objc, Tcl_Obj * const objv[]);
static int BiotextPasteCmd(Biotext *bioPtr, Tcl_Interp *interp, int objc, Tcl_Obj * const objv[]);
static int BiotextGetIndex(Biotext *bioPtr, char *str, int *row_p, int *col_p);
static int BiotextInitBuffer(Biotext *bioPtr, int nseq);
static int BiotextCopySeqsToBuffer(Biotext *bioPtr, int nseqs, Tcl_Obj **OIdx);
static int BiotextAddColsToBufferSeqs(Biotext *bioPtr, int ncols);
static int GetIndexOfTag(Biotext *bioPtr, char *tagName, int mapOrtag);
static int BiotextInsertGapsInTags(Biotext *bioPtr, char *index, int count);

#endif /* TEST_BIOTEXT_MIN_H */
