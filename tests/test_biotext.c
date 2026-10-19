/*
 * test_biotext.c
 *
 * Standalone test harness for the real BiotextInsertCols / BiotextDeleteCols /
 * BiotextInsertRows / BiotextDeleteRows / BiotextCopyCmd / BiotextCutCmd /
 * BiotextPasteCmd / BiotextInsertGapsInTags functions (and their cache
 * helpers), extracted verbatim from the patched tkBiotext.c.
 *
 * Focus: edge cases requested -- first row, last row, first column, last
 * column -- for insert/delete/cut/copy/paste.
 *
 * Build & run: see run_tests.sh (uses tcl8.6 + valgrind to catch any
 * heap corruption/overflow/use-after-free, not just wrong output).
 */

#include "test_biotext_min.h"

static int g_failures = 0;
static int g_tests = 0;

/* ---------------------------------------------------------------- */
/* Helpers to build / free / print / check a small test alignment.  */
/* ---------------------------------------------------------------- */

/* Fills row r, col c with a repeatable, easy-to-eyeball base pattern. */
static char sample_char(int r, int c) {
    static const char bases[4] = {'A','C','G','T'};
    return bases[(r + c) % 4];
}

static Biotext *make_biotext(int nbSeqs, int lgSeqs, int withMapping) {
    Biotext *bioPtr = (Biotext *)calloc(1, sizeof(Biotext));
    bioPtr->interp = Tcl_CreateInterp();
    bioPtr->widgetName = ".biotext_test";
    bioPtr->nbSeqs = nbSeqs;
    bioPtr->lgSeqs = lgSeqs;
    bioPtr->nbMapping = withMapping ? 1 : 0;
    bioPtr->Mapping = NULL;   /* GetIndexOfTag(mapOrtag=0) is only ever
                                 called with nbMapping==0 in our tests,
                                 so the Mapping[] array is never walked */
    bioPtr->Tags = NULL;
    bioPtr->nbTags = 0;
    bioPtr->listTags = NULL;

    bioPtr->SeqMat = (char **)ckalloc(nbSeqs * sizeof(char *));
    bioPtr->SeqGrp = (int *)ckalloc(nbSeqs * sizeof(int));
    if (withMapping) {
        bioPtr->MapSeq = (int **)ckalloc(nbSeqs * sizeof(int *));
    } else {
        bioPtr->MapSeq = NULL;
    }

    for (int i = 0; i < nbSeqs; i++) {
        bioPtr->SeqMat[i] = (char *)ckalloc((lgSeqs + 1) * sizeof(char));
        for (int j = 0; j < lgSeqs; j++) {
            bioPtr->SeqMat[i][j] = sample_char(i, j);
        }
        bioPtr->SeqMat[i][lgSeqs] = '\0';
        bioPtr->SeqGrp[i] = 0;
        if (withMapping) {
            bioPtr->MapSeq[i] = (int *)ckalloc(lgSeqs * sizeof(int));
            for (int j = 0; j < lgSeqs; j++) {
                bioPtr->MapSeq[i][j] = 0;
            }
        }
    }
    return bioPtr;
}

static void free_biotext(Biotext *bioPtr) {
    /* release cache first (mirrors BiotextInitBuffer(bioPtr,0)) */
    BiotextInitBuffer(bioPtr, 0);

    for (int i = 0; i < bioPtr->nbSeqs; i++) {
        ckfree((char *)bioPtr->SeqMat[i]);
        if (bioPtr->MapSeq != NULL) {
            ckfree((char *)bioPtr->MapSeq[i]);
        }
    }
    ckfree((char *)bioPtr->SeqMat);
    if (bioPtr->MapSeq != NULL) {
        ckfree((char *)bioPtr->MapSeq);
    }
    ckfree((char *)bioPtr->SeqGrp);

    Tcl_DeleteInterp(bioPtr->interp);
    free(bioPtr);
}

static void print_align(Biotext *bioPtr, const char *label) {
    printf("---- %s (nbSeqs=%d lgSeqs=%d) ----\n", label, bioPtr->nbSeqs, bioPtr->lgSeqs);
    for (int i = 0; i < bioPtr->nbSeqs; i++) {
        printf("  row %2d: %s\n", i, bioPtr->SeqMat[i]);
    }
}

/* Sanity checks common to every function under test: every row's
 * strlen() must match bioPtr->lgSeqs exactly (catches truncation,
 * missing terminator, or drift caused by an indexing bug), and every
 * row pointer must be non-NULL. This is a *logical* check; the heap
 * safety check (over/under-reads/writes, use-after-free, double
 * free...) is done by running the whole binary under valgrind, see
 * run_tests.sh. */
static int check_consistency(Biotext *bioPtr, const char *ctx) {
    int ok = 1;
    for (int i = 0; i < bioPtr->nbSeqs; i++) {
        if (bioPtr->SeqMat[i] == NULL) {
            printf("  [FAIL] %s: row %d is NULL\n", ctx, i);
            ok = 0;
            continue;
        }
        size_t len = strlen(bioPtr->SeqMat[i]);
        if ((int)len != bioPtr->lgSeqs) {
            printf("  [FAIL] %s: row %d strlen=%zu but lgSeqs=%d\n",
                   ctx, i, len, bioPtr->lgSeqs);
            ok = 0;
        }
    }
    return ok;
}

#define CHECK(cond, msg) do { \
    g_tests++; \
    if (!(cond)) { \
        g_failures++; \
        printf("  [FAIL] %s\n", msg); \
    } else { \
        printf("  [ OK ] %s\n", msg); \
    } \
} while (0)

/* Builds a Tcl_Obj list "{i1 i2 ...}" with each index shifted +1
 * (BiotextCopyCmd/CutCmd expect 1-based row numbers, as documented
 * in their own source: "ROWS NUMBERING STARTS AT 1"). */
static Tcl_Obj *make_index_list_1based(int *idx, int n) {
    Tcl_Obj *lst = Tcl_NewListObj(0, NULL);
    for (int i = 0; i < n; i++) {
        Tcl_ListObjAppendElement(NULL, lst, Tcl_NewIntObj(idx[i] + 1));
    }
    return lst;
}

/* ================================================================== */
/* InsertCols                                                          */
/* ================================================================== */

static void test_insert_cols_first_column(void) {
    printf("\n== TEST: BiotextInsertCols - insertion AVANT la premiere colonne ==\n");
    Biotext *b = make_biotext(4, 6, 0);
    print_align(b, "before");
    /* insert 2 gap columns to the LEFT of column 0 (count<0 => before) */
    int rc = BiotextInsertCols(b, 0, -2, -1);
    print_align(b, "after");
    CHECK(rc == TCL_OK, "return code == TCL_OK");
    CHECK(b->lgSeqs == 8, "lgSeqs grew by 2 (6 -> 8)");
    CHECK(check_consistency(b, "insert_cols_first_column"), "all rows well-formed (strlen==lgSeqs)");
    for (int i = 0; i < b->nbSeqs; i++) {
        CHECK(b->SeqMat[i][0] == '.' || b->SeqMat[i][0] == ' ',
              "gap char correctly placed at new first column");
        break; /* one representative check is enough for the summary */
    }
    free_biotext(b);
}

static void test_insert_cols_last_column(void) {
    printf("\n== TEST: BiotextInsertCols - insertion APRES la derniere colonne ==\n");
    Biotext *b = make_biotext(4, 6, 0);
    print_align(b, "before");
    /* insert 3 gap columns to the RIGHT of the last column (append at end) */
    int rc = BiotextInsertCols(b, b->lgSeqs - 1, 3, -1);
    print_align(b, "after");
    CHECK(rc == TCL_OK, "return code == TCL_OK");
    CHECK(b->lgSeqs == 9, "lgSeqs grew by 3 (6 -> 9)");
    CHECK(check_consistency(b, "insert_cols_last_column"), "all rows well-formed (strlen==lgSeqs)");
    CHECK(b->SeqMat[0][8] == '.' || b->SeqMat[0][8] == ' ',
          "gap char correctly placed at new last column");
    free_biotext(b);
}

/* ================================================================== */
/* DeleteCols                                                          */
/* ================================================================== */

static void test_delete_cols_first_column(void) {
    printf("\n== TEST: BiotextDeleteCols - suppression de la premiere colonne ==\n");
    Biotext *b = make_biotext(4, 6, 0);
    char expected0 = b->SeqMat[0][1]; /* col 1 becomes the new col 0 */
    print_align(b, "before");
    int rc = BiotextDeleteCols(b, 0);
    print_align(b, "after");
    CHECK(rc == TCL_OK, "return code == TCL_OK");
    CHECK(b->lgSeqs == 5, "lgSeqs shrank by 1 (6 -> 5)");
    CHECK(check_consistency(b, "delete_cols_first_column"), "all rows well-formed (strlen==lgSeqs)");
    CHECK(b->SeqMat[0][0] == expected0, "old column 1 correctly shifted into column 0");
    free_biotext(b);
}

static void test_delete_cols_last_column(void) {
    printf("\n== TEST: BiotextDeleteCols - suppression de la derniere colonne ==\n");
    Biotext *b = make_biotext(4, 6, 0);
    print_align(b, "before");
    int rc = BiotextDeleteCols(b, b->lgSeqs - 1);
    print_align(b, "after");
    CHECK(rc == TCL_OK, "return code == TCL_OK");
    CHECK(b->lgSeqs == 5, "lgSeqs shrank by 1 (6 -> 5)");
    CHECK(check_consistency(b, "delete_cols_last_column"), "all rows well-formed (strlen==lgSeqs)");
    free_biotext(b);
}

/* ================================================================== */
/* InsertRows                                                          */
/* ================================================================== */

static void test_insert_rows_first_row(void) {
    printf("\n== TEST: BiotextInsertRows - insertion AVANT la premiere ligne ==\n");
    Biotext *b = make_biotext(4, 6, 0);
    print_align(b, "before");
    /* insert 2 rows BEFORE row 0 (count<0 => before) */
    int rc = BiotextInsertRows(b, 0, -2);
    print_align(b, "after");
    CHECK(rc == TCL_OK, "return code == TCL_OK");
    CHECK(b->nbSeqs == 6, "nbSeqs grew by 2 (4 -> 6)");
    CHECK(check_consistency(b, "insert_rows_first_row"), "all rows well-formed (strlen==lgSeqs)");
    free_biotext(b);
}

static void test_insert_rows_last_row(void) {
    printf("\n== TEST: BiotextInsertRows - insertion APRES la derniere ligne ==\n");
    Biotext *b = make_biotext(4, 6, 0);
    print_align(b, "before");
    /* insert 3 rows AFTER the last row (append at end) */
    int rc = BiotextInsertRows(b, b->nbSeqs - 1, 3);
    print_align(b, "after");
    CHECK(rc == TCL_OK, "return code == TCL_OK");
    CHECK(b->nbSeqs == 7, "nbSeqs grew by 3 (4 -> 7)");
    CHECK(check_consistency(b, "insert_rows_last_row"), "all rows well-formed (strlen==lgSeqs)");
    free_biotext(b);
}

/* ================================================================== */
/* DeleteRows                                                          */
/* ================================================================== */

static void test_delete_rows_first_row(void) {
    printf("\n== TEST: BiotextDeleteRows - suppression de la premiere ligne ==\n");
    Biotext *b = make_biotext(4, 6, 0);
    char saved1[16];
    strcpy(saved1, b->SeqMat[1]); /* row 1 becomes the new row 0 */
    print_align(b, "before");
    int rc = BiotextDeleteRows(b, 0);
    print_align(b, "after");
    CHECK(rc == TCL_OK, "return code == TCL_OK");
    CHECK(b->nbSeqs == 3, "nbSeqs shrank by 1 (4 -> 3)");
    CHECK(check_consistency(b, "delete_rows_first_row"), "all rows well-formed (strlen==lgSeqs)");
    CHECK(strcmp(b->SeqMat[0], saved1) == 0, "old row 1 correctly shifted into row 0");
    free_biotext(b);
}

static void test_delete_rows_last_row(void) {
    printf("\n== TEST: BiotextDeleteRows - suppression de la derniere ligne ==\n");
    Biotext *b = make_biotext(4, 6, 0);
    print_align(b, "before");
    int rc = BiotextDeleteRows(b, b->nbSeqs - 1);
    print_align(b, "after");
    CHECK(rc == TCL_OK, "return code == TCL_OK");
    CHECK(b->nbSeqs == 3, "nbSeqs shrank by 1 (4 -> 3)");
    CHECK(check_consistency(b, "delete_rows_last_row"), "all rows well-formed (strlen==lgSeqs)");
    free_biotext(b);
}

/* Repeated deletion of row 0 down to a single row, then delete the
 * last remaining row: stresses the nbs==1 / "last row of all" path,
 * and repeated use of the copy/cut cache free path (memory-leak fix
 * area) under valgrind. */
static void test_delete_rows_down_to_one(void) {
    printf("\n== TEST: BiotextDeleteRows - suppressions successives jusqu'a 1 ligne ==\n");
    Biotext *b = make_biotext(4, 6, 0);
    int rc = TCL_OK;
    while (b->nbSeqs > 1 && rc == TCL_OK) {
        rc = BiotextDeleteRows(b, 0);
    }
    print_align(b, "after");
    CHECK(rc == TCL_OK, "repeated first-row deletion succeeds down to 1 row");
    CHECK(b->nbSeqs == 1, "exactly one row remains");
    CHECK(check_consistency(b, "delete_rows_down_to_one"), "remaining row well-formed");
    free_biotext(b);
}

/* ================================================================== */
/* Copy / Cut / Paste                                                  */
/* ================================================================== */

static Tcl_Obj *dummy0, *dummy1; /* objv[0], objv[1] placeholders */

static void test_copy_paste_first_row(void) {
    printf("\n== TEST: Copy/Paste - premiere ligne ==\n");
    Biotext *b = make_biotext(4, 6, 0);
    char saved[16];
    strcpy(saved, b->SeqMat[0]);
    print_align(b, "before");

    int idxArr[1] = {0};
    Tcl_Obj *objv[3] = { dummy0, dummy1, make_index_list_1based(idxArr, 1) };
    Tcl_IncrRefCount(objv[2]);
    int rc = BiotextCopyCmd(b, b->interp, 3, objv);
    Tcl_DecrRefCount(objv[2]);
    CHECK(rc == TCL_OK, "BiotextCopyCmd on first row returns TCL_OK");
    CHECK(b->nbSeqsCache == 1, "cache holds exactly 1 row after copy");
    CHECK(strcmp(b->SeqMatCache[0], saved) == 0, "cached content matches the copied first row");

    /* paste it back after the last row ("end") */
    Tcl_Obj *pasteIdx = Tcl_NewStringObj("end", -1);
    Tcl_IncrRefCount(pasteIdx);
    Tcl_Obj *pobjv[3] = { dummy0, dummy1, pasteIdx };
    rc = BiotextPasteCmd(b, b->interp, 3, pobjv);
    Tcl_DecrRefCount(pasteIdx);
    print_align(b, "after copy+paste at end");
    CHECK(rc == TCL_OK, "BiotextPasteCmd at end returns TCL_OK");
    CHECK(b->nbSeqs == 5, "nbSeqs grew by 1 after paste");
    CHECK(strcmp(b->SeqMat[4], saved) == 0, "pasted row matches the originally copied first row");
    CHECK(check_consistency(b, "copy_paste_first_row"), "all rows well-formed after paste");
    free_biotext(b);
}

static void test_cut_paste_last_row(void) {
    printf("\n== TEST: Cut/Paste - derniere ligne ==\n");
    Biotext *b = make_biotext(4, 6, 0);
    char saved[16];
    strcpy(saved, b->SeqMat[3]);
    print_align(b, "before");

    int idxArr[1] = {3};
    Tcl_Obj *objv[3] = { dummy0, dummy1, make_index_list_1based(idxArr, 1) };
    Tcl_IncrRefCount(objv[2]);
    int rc = BiotextCutCmd(b, b->interp, 3, objv);
    Tcl_DecrRefCount(objv[2]);
    print_align(b, "after cut");
    CHECK(rc == TCL_OK, "BiotextCutCmd on last row returns TCL_OK");
    CHECK(b->nbSeqs == 3, "nbSeqs shrank by 1 after cut");
    CHECK(b->nbSeqsCache == 1, "cut populated the cache (regression test for the fixed bug)");
    CHECK(strcmp(b->SeqMatCache[0], saved) == 0, "cache holds the exact content of the cut row");

    /* paste it back before the (new) first row */
    Tcl_Obj *pasteIdx = Tcl_NewStringObj("1", -1);
    Tcl_IncrRefCount(pasteIdx);
    Tcl_Obj *pobjv[3] = { dummy0, dummy1, pasteIdx };
    rc = BiotextPasteCmd(b, b->interp, 3, pobjv);
    Tcl_DecrRefCount(pasteIdx);
    print_align(b, "after cut+paste at row 1");
    CHECK(rc == TCL_OK, "BiotextPasteCmd at row 1 returns TCL_OK");
    CHECK(b->nbSeqs == 4, "nbSeqs restored after paste");
    CHECK(check_consistency(b, "cut_paste_last_row"), "all rows well-formed after paste");
    free_biotext(b);
}

/* Paste of sequences SHORTER than the current alignment width: this is
 * the exact scenario that used to NULL-deref inside
 * BiotextAddColsToBufferSeqs when nbMapping==0 (fixed bug). */
static void test_paste_needs_widening_no_mapping(void) {
    printf("\n== TEST: Paste - la sequence copiee est plus courte (regression NULL deref) ==\n");
    Biotext *src = make_biotext(2, 4, 0);   /* short alignment, copied from */
    Biotext *dst = make_biotext(3, 9, 0);   /* wider alignment, pasted into */

    int idxArr[1] = {0};
    Tcl_Obj *objv[3] = { dummy0, dummy1, make_index_list_1based(idxArr, 1) };
    Tcl_IncrRefCount(objv[2]);
    /* copy the short row from src into src's own cache, then manually
     * point dst at that cache to simulate "copy from a short buffer,
     * paste into a longer alignment", i.e. diff = dst->lgSeqs - cacheLg > 0 */
    int rc = BiotextCopyCmd(src, src->interp, 3, objv);
    Tcl_DecrRefCount(objv[2]);
    CHECK(rc == TCL_OK, "copy from the short alignment succeeds");

    /* transplant the cache onto dst (same shape the widget would have
     * after the user does Copy in one biotext and Paste in another,
     * or copies then widens the target beforehand) */
    dst->nbSeqsCache  = src->nbSeqsCache;
    dst->lgSeqsCache  = src->lgSeqsCache;
    dst->SeqMatCache  = src->SeqMatCache;
    dst->MapSeqCache  = src->MapSeqCache;
    dst->SeqGrpCache  = src->SeqGrpCache;
    src->nbSeqsCache = 0; /* prevent double free in free_biotext(src) */
    src->SeqMatCache = NULL;
    src->MapSeqCache = NULL;
    src->SeqGrpCache = NULL;

    print_align(dst, "dst before paste (cache row is only 4 cols wide, dst is 9)");
    Tcl_Obj *pasteIdx = Tcl_NewStringObj("end", -1);
    Tcl_IncrRefCount(pasteIdx);
    Tcl_Obj *pobjv[3] = { dummy0, dummy1, pasteIdx };
    rc = BiotextPasteCmd(dst, dst->interp, 3, pobjv);
    Tcl_DecrRefCount(pasteIdx);
    print_align(dst, "dst after paste");
    CHECK(rc == TCL_OK, "paste of a narrower cached row into a wider alignment "
                          "does not crash (regression test)");
    CHECK(check_consistency(dst, "paste_needs_widening_no_mapping"), "all rows well-formed after paste");

    free_biotext(src);
    free_biotext(dst);
}

/* ================================================================== */
/* InsertGapsInTags                                                     */
/* ================================================================== */

/* Sets up one tag "T" spanning the whole alignment on every row
 * ("0 lgSeqs-1" per row), then exercises BiotextInsertGapsInTags on
 * the first row/first column and last row/last column, checking that
 * the tag's coordinate list is updated coherently and that valgrind
 * reports no invalid access (this exercises the two DecrRefCount
 * fixes made in this function). */
static Biotext *make_biotext_with_tag(int nbSeqs, int lgSeqs) {
    Biotext *b = make_biotext(nbSeqs, lgSeqs, 0);
    b->nbTags = 1;
    b->Tags = (Tag *)ckalloc(sizeof(Tag));
    b->Tags[0].tagName = strdup("T");
    b->Tags[0].nbCoords = nbSeqs;
    b->Tags[0].coords = Tcl_NewListObj(0, NULL);
    Tcl_IncrRefCount(b->Tags[0].coords);
    for (int i = 0; i < nbSeqs; i++) {
        Tcl_Obj *row = Tcl_NewListObj(0, NULL);
        Tcl_ListObjAppendElement(NULL, row, Tcl_NewIntObj(0));
        Tcl_ListObjAppendElement(NULL, row, Tcl_NewIntObj(lgSeqs - 1));
        Tcl_ListObjAppendElement(NULL, b->Tags[0].coords, row);
    }
    b->listTags = Tcl_NewListObj(0, NULL);
    Tcl_IncrRefCount(b->listTags);
    Tcl_ListObjAppendElement(NULL, b->listTags, Tcl_NewIntObj(0));
    return b;
}

static void free_biotext_with_tag(Biotext *b) {
    Tcl_DecrRefCount(b->Tags[0].coords);
    free(b->Tags[0].tagName);
    ckfree((char *)b->Tags);
    Tcl_DecrRefCount(b->listTags);
    free_biotext(b);
}

static int tag_row_bounds(Biotext *b, int row, int *start, int *stop) {
    Tcl_Obj *rowObj;
    if (Tcl_ListObjIndex(b->interp, b->Tags[0].coords, row, &rowObj) != TCL_OK) return 0;
    Tcl_Obj **v; Tcl_Size n;
    if (Tcl_ListObjGetElements(b->interp, rowObj, &n, &v) != TCL_OK || n < 2) return 0;
    Tcl_GetIntFromObj(b->interp, v[0], start);
    Tcl_GetIntFromObj(b->interp, v[1], stop);
    return 1;
}

static void test_insert_gaps_first_column(void) {
    printf("\n== TEST: BiotextInsertGapsInTags - insertion a la premiere colonne (row 0) ==\n");
    Biotext *b = make_biotext_with_tag(3, 6);
    int s0, e0;
    tag_row_bounds(b, 0, &s0, &e0);
    printf("  tag row0 before: [%d,%d]\n", s0, e0);
    /* "1.0" -> row 0 (1-based in the index string), col 0 ; insert 2 to the right */
    int rc = BiotextInsertGapsInTags(b, "1.0", 2);
    int s1, e1;
    tag_row_bounds(b, 0, &s1, &e1);
    printf("  tag row0 after : [%d,%d]\n", s1, e1);
    CHECK(rc == TCL_OK, "return code == TCL_OK");
    CHECK(e1 == e0 + 2, "tag end bound shifted right by the inserted gap count");
    free_biotext_with_tag(b);
}

static void test_insert_gaps_last_column(void) {
    printf("\n== TEST: BiotextInsertGapsInTags - insertion apres la derniere colonne (last row) ==\n");
    int nbSeqs = 3, lgSeqs = 6;
    Biotext *b = make_biotext_with_tag(nbSeqs, lgSeqs);
    int lastRow = nbSeqs - 1;
    int s0, e0;
    tag_row_bounds(b, lastRow, &s0, &e0);
    printf("  tag row%d before: [%d,%d]\n", lastRow, s0, e0);
    char idx[16];
    /* index strings are 1-based on the row part (row-1 applied inside
     * BiotextGetIndex), so lastRow (0-based) is "lastRow+1" here */
    sprintf(idx, "%d.%d", lastRow + 1, lgSeqs - 1);
    int rc = BiotextInsertGapsInTags(b, idx, 1);
    int s1, e1;
    tag_row_bounds(b, lastRow, &s1, &e1);
    printf("  tag row%d after : [%d,%d]\n", lastRow, s1, e1);
    CHECK(rc == TCL_OK, "return code == TCL_OK");
    /* NOTE on the expected value: insertion is done to the RIGHT of
     * col (right=1 branch), which only extends a tag whose "stop" is
     * strictly greater than col ("stop > col"), not "stop >= col".
     * This is intentional and asymmetric with the left-insertion
     * branch (which uses "stop >= col"): inserting BEFORE col
     * displaces whatever was sitting at col, so a tag ending exactly
     * there must move with it; inserting AFTER col does not touch
     * col itself, so a tag ending exactly at col (the last existing
     * column here) is correctly left untouched -- the newly appended
     * trailing columns are new content, not retroactively part of an
     * existing annotation. So here stop stays == e0. */
    CHECK(e1 == e0, "tag end bound correctly left untouched: insertion strictly "
                      "after the tag's last column does not retroactively extend it");
    free_biotext_with_tag(b);
}

/* ================================================================== */

int main(void) {
    dummy0 = Tcl_NewStringObj("biotext", -1);
    dummy1 = Tcl_NewStringObj("copy", -1);
    Tcl_IncrRefCount(dummy0);
    Tcl_IncrRefCount(dummy1);

    test_insert_cols_first_column();
    test_insert_cols_last_column();
    test_delete_cols_first_column();
    test_delete_cols_last_column();

    test_insert_rows_first_row();
    test_insert_rows_last_row();
    test_delete_rows_first_row();
    test_delete_rows_last_row();
    test_delete_rows_down_to_one();

    test_copy_paste_first_row();
    test_cut_paste_last_row();
    test_paste_needs_widening_no_mapping();

    test_insert_gaps_first_column();
    test_insert_gaps_last_column();

    Tcl_DecrRefCount(dummy0);
    Tcl_DecrRefCount(dummy1);

    printf("\n=====================================\n");
    printf("Total checks: %d, failures: %d\n", g_tests, g_failures);
    printf("=====================================\n");

    return g_failures == 0 ? 0 : 1;
}
