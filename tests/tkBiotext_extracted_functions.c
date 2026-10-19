/* ==== BiotextInsertCols (from line 3092) ==== */
static int
BiotextInsertCols(register Biotext *bioPtr,
		  int index,
		  int count,
		  int row) 
{
    int i, j, iMap, iSpc, iIns, cIns, lgs, nseq;
    int stop1, stop2, start2;
  
    lgs  = bioPtr->lgSeqs + abs(count);
    nseq = bioPtr->nbSeqs;
#ifdef DEBUG_INSERTCOLS
    fprintf(stderr, "\n== in BiotextInsertCols =========\n");
    fprintf(stderr, "widget %s\n", bioPtr->widgetName);
    fprintf(stderr, "index %d count %d row %d\n", index, count, row);
    fprintf(stderr, "nbSeqs %d lgSeqs %d\n", bioPtr->nbSeqs, bioPtr->lgSeqs);
    fflush(stderr);
#endif

    /* 
     * First :
     * Reallocate sequence, mapping and matrices at the new length
     */
    for (i=0;i<nseq;i++) {
	bioPtr->SeqMat[i] = (char *)ckrealloc((char *)bioPtr->SeqMat[i],(lgs+1)*sizeof(char));
	if (bioPtr->nbMapping > 0) {
	    bioPtr->MapSeq[i]   = (int *)ckrealloc((char *)bioPtr->MapSeq[i],lgs*sizeof(int));
	}
    }
    bioPtr->lgSeqs = lgs;
#ifdef DEBUG_INSERTCOLS
    fprintf(stderr, "apres alloc, lgSeqs is now %d\n", bioPtr->lgSeqs);
    fflush(stderr);
#endif
  
    /*
     * Second :
     * find tag for gap and space characters
     */
    iMap = GetIndexOfTag(bioPtr, ".", 0);
    iSpc = GetIndexOfTag(bioPtr, " ", 0);

    /* 
     * Third :
     * Do the insertion before or after index
     */
    if (count < 0) {
	/* insert at the left of index */
	/*      v           v 
	 * ABCDEF..   .ABCDEF..
	 * 01234567   012345678
	 */
	count = abs(count);
	stop1 = index + count;
	start2 = index;
	stop2 = index + count;
    }  else {
	/* insert at the right of index */
	stop1 = index + count;
	start2 = index + 1;
	stop2 = index + count + 1;
    }
#ifdef DEBUG_INSERTCOLS
    fprintf(stderr, "stop1 %d start2 %d stop2 %d\n", stop1, start2, stop2);
    fflush(stderr);
#endif
    for (j=0; j<nseq; j++) {
	if (bioPtr->SeqMat[j][0] == 32) {
	    cIns = 32;
	    iIns = iSpc;
	} else {
	    cIns = 46;
	    iIns = iMap;
	}
#ifdef DEBUG_INSERTCOLS
	if (j==1) {
	    fprintf(stderr,"in  %lu >%s<\n", strlen(bioPtr->SeqMat[j]), bioPtr->SeqMat[j]);
	    fflush(stderr);
	}
#endif
	//strcpy(bioPtr->SeqMat[j]+stop1, bioPtr->SeqMat[j]+start1);
	/* Bug fix: SeqMat[j] was reallocated to (lgs+1) bytes just above,
	 * so valid indices are 0..lgs. Starting the shift at i=lgs+1 wrote
	 * one byte past the end of the buffer (heap overflow) on every
	 * call. The loop must start at i=lgs (the new terminator slot). */
	for (i=lgs; i>=stop1; i--) {
	    bioPtr->SeqMat[j][i] = bioPtr->SeqMat[j][i-count];
	}
#ifdef DEBUG_INSERTCOLS
	if (j==1) {
	    fprintf(stderr, "out %lu >%s<\n", strlen(bioPtr->SeqMat[j]), bioPtr->SeqMat[j]);
	    fflush(stderr);
	}
#endif
    }
    //DumpNullChar(bioPtr);
    if (bioPtr->nbMapping > 0) {
	for (j=0; j<bioPtr->nbSeqs; j++) {
	    for (i=lgs-1; i>stop1; i--) {
		bioPtr->MapSeq[j][i]   = bioPtr->MapSeq[j][i-count];
	    }
	}
    }

#ifdef DEBUG_INSERTCOLS  
    fprintf(stderr, "\nstart2 %d stop2 %d\n", start2, stop2);
    fflush(stderr);
#endif

    for (j=0; j<bioPtr->nbSeqs; j++) {
	if (bioPtr->SeqMat[j][0] == 32) {
	    cIns = 32;
	    iIns = iSpc;
	} else {
	    cIns = 46;
	    iIns = iMap;
	}
	for (i=start2; i<stop2; i++) {
	    bioPtr->SeqMat[j][i] = cIns;
	    if (bioPtr->nbMapping > 0) {
		bioPtr->MapSeq[j][i] = iIns;
	    }
	}
#ifdef DEBUG_INSERTCOLS
	if (j==1) {
	    fprintf(stderr, "gap %lu >%s<\n", strlen(bioPtr->SeqMat[j]), bioPtr->SeqMat[j]);
	    fflush(stderr);
	}
#endif
    }  

    /*
     * insert gaps in tags
     */
    if (bioPtr->Tags != NULL) {
	//DumpCoordsTag(bioPtr, "entre insertcols");
	char pos[64];
	int i1, i2;

	if (row == -1) {
	    i1 = 0;
	    i2 = nseq;
	} else {
	    i1 = row;
	    i2 = row+1;
	}

	for (i=i1; i<i2; i++) {
	    /* Bug fix: "i" is a 0-based row counter here, but the index
	     * string parsed by BiotextGetIndex() (via
	     * BiotextInsertGapsInTags) expects a 1-based row number (it
	     * does "row -= 1" internally). Passing "i" directly meant
	     * every row's index string was off by one: row 0 (the first
	     * row) resolved, after the "-1", to row -1 which appears to
	     * get clamped back to row 0 -- so the first row's tags were
	     * updated on EVERY loop iteration (once per row in the
	     * alignment) instead of just once, while every other row's
	     * tags were never updated at all. Confirmed via a live test:
	     * inserting columns shifted row 1's tag by 2x the requested
	     * gap count, and row 2's tag didn't shift at all. */
	    sprintf(pos, "%d.%d", i+1, index);
	    BiotextInsertGapsInTags(bioPtr, pos, count);
	}
	//DumpCoordsTag(bioPtr, "sort insertcols");
    }
  
    return TCL_OK;
}

/* ==== BiotextInsertRows (from line 3278) ==== */
static int
BiotextInsertRows(register Biotext *bioPtr,
		  int index,
		  int count) 
{
    Tcl_Interp *interp = bioPtr->interp;
    int i, j, nbs, iMap, before, len;
    int stop1, stop2, start;
    Tcl_Obj *tmpObj, *coordsObj;
    Tcl_Size lenSz;
  
    nbs = bioPtr->nbSeqs + abs(count);

    /* realloc matrices to the desired number of rows */
    bioPtr->SeqMat = (char **)ckrealloc((char *)bioPtr->SeqMat, nbs*sizeof(char *));
    if (bioPtr->nbMapping > 0) {
	bioPtr->MapSeq = (int **)ckrealloc((char *)bioPtr->MapSeq, nbs*sizeof(int *));
    }
    bioPtr->SeqGrp = (int *)ckrealloc((char *)bioPtr->SeqGrp, nbs*sizeof(int));
  
    /* Allocate the added sequence to the 
     * actual length of the alignment
     */
    for (i=bioPtr->nbSeqs; i<nbs; i++) {
	bioPtr->SeqMat[i] = (char *)ckalloc((bioPtr->lgSeqs+1)*sizeof(char));
	if (bioPtr->nbMapping > 0) {
	    bioPtr->MapSeq[i] = (int *)ckalloc(bioPtr->lgSeqs*sizeof(int));
	}
    }

    /* mapping index for insertion character */
    //iMap = GetIndexOfTag(bioPtr, ".", 0);
    iMap = GetIndexOfTag(bioPtr, " ", 0);
  
    /*
     * count < 0 insert BEFORE index,
     * count > 0 insert AFTER index
     */
    if (count < 0) {
	count = abs(count);
	before = 1;
    } else {
	before = 0;
    }

    if (before) {
	stop1 = index + count - 1;
	start = index;
	stop2 = index + count;
    }  else {
	stop1 = index + count;
	start = index + 1;
	stop2 = index + count + 1;
    }

    for (i=nbs-1; i>stop1; i--) {
	bioPtr->SeqGrp[i] = bioPtr->SeqGrp[i-count];
	strcpy(bioPtr->SeqMat[i], bioPtr->SeqMat[i-count]);
	if (bioPtr->nbMapping > 0) {
	    for (j=0; j<bioPtr->lgSeqs; j++) {
		bioPtr->MapSeq[i][j] = bioPtr->MapSeq[i-count][j];
	    }
	}
    }
  
    for (i=start; i<stop2; i++) {
	bioPtr->SeqGrp[i] = 0;
	for (j=0; j<bioPtr->lgSeqs; j++) {
	    bioPtr->SeqMat[i][j] = iMap;
	    if (bioPtr->nbMapping > 0) {
		bioPtr->MapSeq[i][j] = iMap;
	    }
	}
	bioPtr->SeqMat[i][bioPtr->lgSeqs] = '\0';
    }

    /* insert tags if necessary */
    if (bioPtr->Tags != NULL) {

#ifdef DEBUG_INSERTROW
	fprintf(stderr, "== in BiotextInsertRow ==========\n");
	fprintf(stderr, "widget %s\n", bioPtr->widgetName);
	fprintf(stderr, "Insertion at index %d\n", index);
	fflush(stderr);
#endif
	tmpObj = Tcl_NewListObj(0, NULL);
	Tcl_Obj *tmpObj3[count];
	for (i=0; i<count; i++) {
	    Tcl_ListObjAppendElement(NULL, tmpObj, Tcl_NewListObj(0, NULL));
	    tmpObj3[i] = Tcl_NewListObj(0, NULL);
	}
    
	for (i=0; i<bioPtr->nbTags; i++) {
#ifdef DEBUG_INSERTROW
	    fprintf(stderr,"Insert in tag %d : %s\n", i, bioPtr->Tags[i].sname);
#endif
	    coordsObj = Tcl_DuplicateObj(bioPtr->Tags[i].coords);
	    Tcl_ListObjLength(NULL, coordsObj, &lenSz);
	    len = (int)lenSz;
#ifdef DEBUG_INSERTROW
	    fprintf(stderr, "nb coords AVANT : %d\n", len);
	    fflush(stderr);
#endif
	    Tcl_ListObjReplace(interp, coordsObj, index+1, 0, count, tmpObj3);
	    bioPtr->Tags[i].coords = Tcl_DuplicateObj(coordsObj);
#ifdef DEBUG_INSERTROW
	    Tcl_ListObjLength(NULL, bioPtr->Tags[i].coords, &lenSz);
	    len = (int)lenSz;
	    fprintf(stderr, "len APRES : %d\n", len);
	    fflush(stderr);
#endif
	}
    }
  
    bioPtr->nbSeqs = nbs;

    return TCL_OK;
}

/* ==== BiotextDeleteColsGaps (from line 3531) ==== */
static int
BiotextDeleteColsGaps(register Biotext *bioPtr) 
{
    int i, j, isGap, *colToDel, c;
    char posChar;
  
    colToDel = (int *)ckalloc(bioPtr->lgSeqs*sizeof(int));
    for (i=bioPtr->lgSeqs-1;i>=0;i--) {
	isGap = 1;
	for (j=0;j<bioPtr->nbSeqs;j++) {
	    posChar = bioPtr->SeqMat[j][i];
	    if (posChar != '.' && posChar != ' ') {
		isGap = 0;
		break;
	    }
	}
	colToDel[i] = isGap;
    }

    c = bioPtr->lgSeqs-1;
    for (i=c;i>=0;i--) {
	if (colToDel[i] == 1) {
	    if (BiotextDeleteCols(bioPtr, i) != TCL_OK) {
		fprintf(stderr, "error deleting col %d\n", i);
	    }
	}
    }
    ckfree((char *)colToDel);
   
    return TCL_OK;
}

/* ==== BiotextDeleteCols (from line 3580) ==== */
static int
BiotextDeleteCols(register Biotext *bioPtr,
		  int index) 
{
    Tcl_Interp *interp = bioPtr->interp;
    int i, j, lgs, nseq;

    lgs  = bioPtr->lgSeqs;
    nseq = bioPtr->nbSeqs;

#ifdef DEBUG_DELCOLS
    fprintf(stderr, "\n== BiotextDeleteCols ========\n");
    fprintf(stderr, "widget %s\n", bioPtr->widgetName);
    fflush(stderr);
#endif
    for (j=0;j<nseq;j++) {
	if (index != lgs-1) {
	    //strcpy(bioPtr->SeqMat[j]+index, bioPtr->SeqMat[j]+index+1);
	    /* Shift chars left by one, including the trailing '\0'.
	     * SeqMat[j] holds valid indices 0..lgs (lgs+1 bytes),
	     * so the loop must stop at i == lgs-1 (reads i+1 == lgs,
	     * which is the terminator). Going up to i == lgs would
	     * read SeqMat[j][lgs+1], which is out of bounds. */
	    for (i=index;i<lgs;i++) {
		bioPtr->SeqMat[j][i] = bioPtr->SeqMat[j][i+1];
	    }
	    if (bioPtr->nbMapping) {
		for (i=index;i<lgs-1;i++) {
		    bioPtr->MapSeq[j][i] = bioPtr->MapSeq[j][i+1];
		}
	    }
	}

	/* SeqMat was allocated to lgSeqs+1; new length is lgs-1,
	 * so the new buffer needs lgs bytes (lgs-1 chars + '\0'). */
	bioPtr->SeqMat[j] = (char *)ckrealloc((char *)bioPtr->SeqMat[j], lgs*sizeof(char)); 
	/* NOTE: bioPtr->lgSeqs has NOT been decremented yet at this point
	 * (it is only updated later, after this loop, around line 3258).
	 * Using bioPtr->lgSeqs here as an index would write to offset
	 * lgs in a buffer of size lgs (valid indices 0..lgs-1) -> heap
	 * buffer overflow. The correct terminator position is lgs-1. */
	bioPtr->SeqMat[j][lgs-1] = '\0';
	/* Bug fix: unlike every other MapSeq access in this file, this
	 * realloc was not guarded by "if (bioPtr->nbMapping)". MapSeq is
	 * only ever allocated when nbMapping > 0; with no mapping active
	 * (the common case), bioPtr->MapSeq is NULL and bioPtr->MapSeq[j]
	 * dereferences NULL -> immediate crash on every column deletion. */
	if (bioPtr->nbMapping) {
	    bioPtr->MapSeq[j] = (int *)ckrealloc((int *)bioPtr->MapSeq[j], (lgs-1)*sizeof(int)); 
	}
    }

    /* change tags limits if necessary*/
    int k, nrows, nelt, start, stop;
    Tcl_Size nrowsSz, neltSz;
    Tcl_Obj *coordsObj, *newRowObj, *rowObj, *newCoordsObj, **zonesObj;

#ifdef DEBUG_DELCOLS
    fprintf(stderr,"nbTags %d\n", bioPtr->nbTags);
#endif
    for (i=0; i< bioPtr->nbTags; i++) {
	coordsObj = Tcl_DuplicateObj(bioPtr->Tags[i].coords);
	Tcl_ListObjLength(interp, coordsObj, &nrowsSz);
	nrows = (int)nrowsSz;
#ifdef DEBUG_DELCOLS
	fprintf(stderr, "coords for %d rows\n", nrows);
	fflush(stderr);
#endif
	newCoordsObj = Tcl_NewListObj(0, NULL);
	for (j=0; j<nrows; j++) {
	    /* Bug fix: must index by the row counter j (the row within
	     * this tag's coords list), not by i (the outer tag index).
	     * Using i here read the wrong row (or errored out silently
	     * once i >= nrows for a given tag). */
	    Tcl_ListObjIndex(interp, coordsObj, j, &rowObj);
	    Tcl_ListObjGetElements(interp, rowObj, &neltSz, &zonesObj);
	    nelt = (int)neltSz;
	    newRowObj = Tcl_NewListObj(0, NULL);
	    if (nelt > 0) {
		for (k=0; k<nelt; k+=2) {
		    Tcl_GetIntFromObj(interp, zonesObj[k], &start);
		    Tcl_GetIntFromObj(interp, zonesObj[k+1], &stop);
		    if (index <= stop) {
			stop--;
		    }
		    if (index < start) {
			start--;
		    }
		    Tcl_ListObjAppendElement(interp, newRowObj, Tcl_NewIntObj(start));
		    Tcl_ListObjAppendElement(interp, newRowObj, Tcl_NewIntObj(stop));
		}
	    }
	    Tcl_ListObjAppendElement(interp, newCoordsObj, Tcl_DuplicateObj(newRowObj));
	}
	bioPtr->Tags[i].coords = Tcl_DuplicateObj(newCoordsObj);
    }

    bioPtr->lgSeqs -= 1;
  
    if (bioPtr->currentC >= index) {
	bioPtr->currentC-=1;
    }

    return TCL_OK;
}

/* ==== BiotextDeleteRows (from line 3702) ==== */
static int
BiotextDeleteRows(register Biotext *bioPtr,
		  int index) 
{
    Tcl_Interp *interp = bioPtr->interp;
    int i, j, k, nbs;
    Tcl_Obj *coordsObj;
  
    nbs = bioPtr->nbSeqs;
    for (i=index; i<nbs-1; i++) {
	bioPtr->SeqGrp[i] = bioPtr->SeqGrp[i+1];
	strcpy(bioPtr->SeqMat[i], bioPtr->SeqMat[i+1]);
	if (bioPtr->nbMapping > 0) {
	    for (j=0; j<bioPtr->lgSeqs; j++) {
		bioPtr->MapSeq[i][j] = bioPtr->MapSeq[i+1][j];
	    }
	}
    }

    if (bioPtr->Tags != NULL) {
	for(k=0; k<bioPtr->nbTags; k++) {
	    coordsObj = Tcl_DuplicateObj(bioPtr->Tags[k].coords);
	    Tcl_ListObjReplace(interp, coordsObj, index, 1, 0, NULL);
	    bioPtr->Tags[k].coords = Tcl_DuplicateObj(coordsObj);
	}
    }

    /* Bug fix (memory leak): after the shift loop above, the buffers at
     * the old last row index (nbs-1) still hold their own allocation
     * (a duplicate of the row before them); nothing else points to it
     * anymore once the pointer arrays are shrunk below. Free them here
     * before shrinking bioPtr->SeqMat / bioPtr->MapSeq, otherwise this
     * allocation is silently leaked on every row deletion. */
    ckfree((char *)bioPtr->SeqMat[nbs-1]);
    if (bioPtr->nbMapping > 0) {
	ckfree((char *)bioPtr->MapSeq[nbs-1]);
    }

    bioPtr->nbSeqs -= 1;
  
    /* desallocate one sequence, group, and mapping */
    bioPtr->SeqMat = (char **)ckrealloc((char *)bioPtr->SeqMat, bioPtr->nbSeqs*sizeof(char *));
    bioPtr->SeqGrp = (int *)ckrealloc((char *)bioPtr->SeqGrp, bioPtr->nbSeqs*sizeof(int));
    if (bioPtr->nbMapping > 0) {
	bioPtr->MapSeq = (int **)ckrealloc((char *)bioPtr->MapSeq, bioPtr->nbSeqs*sizeof(int *));
    }

    if (index <= bioPtr->currentR) {
	bioPtr->currentR--;
    }  
  
    return TCL_OK;
}

/* ==== BiotextCopyCmd (from line 4417) ==== */
static int
BiotextCopyCmd(register Biotext *bioPtr,
	       Tcl_Interp *interp,
	       int objc,
	       Tcl_Obj * const objv[]) 
{
    int nseq, i, val;
    Tcl_Size nseqSz;
    Tcl_Obj **OIdx, **newIdx, *evalObj, *resObj;

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 2, objv, " copy <index-list>");
	return TCL_ERROR;
    }
  
    Tcl_ListObjLength(interp,objv[2],&nseqSz);
    nseq = (int)nseqSz;
    if (nseq == 0) {
	return TCL_OK;
    }
  
    /*
     * sort the index list in decreasing order
     */
    evalObj = Tcl_NewListObj(0, NULL);
    Tcl_ListObjAppendElement(NULL, evalObj, Tcl_NewStringObj("lsort", -1));
    Tcl_ListObjAppendElement(NULL, evalObj, Tcl_NewStringObj("-integer", -1));
    Tcl_ListObjAppendElement(NULL, evalObj, Tcl_NewStringObj("-decreasing", -1));
    Tcl_ListObjAppendElement(NULL, evalObj, objv[2]);

    if (Tcl_EvalObjEx(interp, evalObj, TCL_EVAL_DIRECT) != TCL_OK) {
	return TCL_ERROR;
    }
  
    resObj = Tcl_NewListObj(0, NULL);
    resObj = Tcl_GetObjResult(interp);
    Tcl_ListObjGetElements(interp, resObj, &nseqSz, &OIdx);
    nseq = (int)nseqSz;
    Tcl_ResetResult(interp);

    /*
     * Lines indexes start at 1
     * Re-create index table starting at 0
     */
    resObj = Tcl_NewListObj(0, NULL);
    for(i=0; i<nseq; i++) {
	Tcl_GetIntFromObj(NULL, OIdx[i], &val);
	Tcl_ListObjAppendElement(NULL, resObj, Tcl_NewIntObj(val - 1));
    }

    Tcl_ListObjGetElements(NULL, resObj, &nseqSz, &newIdx);
    nseq = (int)nseqSz;
  
    /*
     * Initialize the copy buffer. 
     * Only one step backcopy/paste allowed at present
     */
    BiotextInitBuffer(bioPtr, nseq);
    BiotextCopySeqsToBuffer(bioPtr, nseq, newIdx);
    //DumpCache(bioPtr, "end of Copycmd");
    return TCL_OK;
}

/* ==== BiotextCutCmd (from line 4498) ==== */
static int
BiotextCutCmd(register Biotext *bioPtr,
	      Tcl_Interp *interp,
	      int objc,
	      Tcl_Obj * const objv[]) 
{
    int nseq, i, idx, val;
    Tcl_Size nseqSz;
    Tcl_Obj **tmpOIdx, **OIdx, *resObj, *evalObj, *tmpObj;
  
    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "<index-list>");
	return TCL_ERROR;
    }

    if (Tcl_ListObjLength(interp, objv[2], &nseqSz) != TCL_OK) {
	return TCL_ERROR;
    }
    nseq = (int)nseqSz;
    if (nseq == 0) {
	return TCL_OK;
    }

    /*
     * sort the index list in decreasing order
     */
    /*
     * BEWARE !
     * ROWS NUMBERING STARTS AT 1 !
     * So here row = row - 1 !
     */
    evalObj = Tcl_NewListObj(0, NULL);
    Tcl_ListObjAppendElement(NULL, evalObj, Tcl_NewStringObj("lsort", -1));
    Tcl_ListObjAppendElement(NULL, evalObj, Tcl_NewStringObj("-integer", -1));
    Tcl_ListObjAppendElement(NULL, evalObj, Tcl_NewStringObj("-decreasing", -1));
    Tcl_ListObjAppendElement(NULL, evalObj, objv[2]);
  
    if (Tcl_EvalObjEx(interp, evalObj, TCL_EVAL_DIRECT) != TCL_OK) {
	return TCL_ERROR;
    }
  
    resObj = Tcl_NewListObj(0, NULL);
    resObj = Tcl_GetObjResult(interp);
    if (Tcl_ListObjGetElements(interp, resObj, &nseqSz, &tmpOIdx) != TCL_OK) {
	return TCL_ERROR;
    }
    nseq = (int)nseqSz;
    tmpObj = Tcl_NewListObj(0, NULL);
    for (i=0; i<nseq; i++) {
	Tcl_GetIntFromObj(interp, tmpOIdx[i], &val);
	/* substract 1 to val as indices starts at 0 here */
	Tcl_ListObjAppendElement(interp, tmpObj, Tcl_NewIntObj(val-1));
    }
    if (Tcl_ListObjGetElements(interp, tmpObj, &nseqSz, &OIdx) != TCL_OK) {
	return TCL_ERROR;
    }
    nseq = (int)nseqSz;

    /*
     * Initialize the copy buffer. 
     * Only one step back
     * copy/paste allowed at present
     */
    /* Bug fix: these two calls were left commented out (debug leftover),
     * so "cut" never populated the copy/paste cache before deleting the
     * rows below. A subsequent "paste" would then restore whatever was
     * last "copy"'d (or nothing at all), silently losing the cut data.
     * "cut" must behave as copy-then-delete, so these are restored. */
    //DumpCoordsTag(bioPtr, "CutCmd\nAVANT InitBuffer");
    BiotextInitBuffer(bioPtr, nseq);
    //DumpCoordsTag(bioPtr, "APRES InitBuffer, AVANT CopySeqsToBuffer");
    BiotextCopySeqsToBuffer(bioPtr, nseq, OIdx);
    //DumpCoordsTag(bioPtr, "APRES CopySeqsToBuffer");

    /*
     * Now delete the cut sequences 
     */

    for (i=0; i<nseq; i++) {
	Tcl_GetIntFromObj(interp, OIdx[i], &idx);
	//fprintf(stderr, "deleteing row %d\n", idx);
	//fflush(stderr);
	BiotextDeleteRows(bioPtr, idx);
    }
    //DumpSeqs(bioPtr, "CutSeqs done\n");
    //DumpCache(bioPtr, "CutSeqs done\n");
    /* redraw the widget */
    bioPtr->flags |= UPDATE_SCROLLBAR;
    BiotextEventuallyRedraw(bioPtr);

    return TCL_OK;
}

/* ==== BiotextPasteCmd (from line 4608) ==== */
static int
BiotextPasteCmd(register Biotext *bioPtr,
		Tcl_Interp *interp,
		int objc,
		Tcl_Obj * const objv[]) 
{
    int idx, i, j, k, diff;
    char *res;

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "index");
	return TCL_ERROR;
    }
  
    /*
     * If nothing to paste, return
     */
    if (bioPtr->nbSeqsCache == 0) {
	return TCL_OK;
    }

    /*
     * Get the insertion point.
     */
    res = Tcl_GetString(objv[2]);
    if (strcasecmp(res, "end") == 0) {
	idx = bioPtr->nbSeqs-1;
    } else {
	if (Tcl_GetInt(interp, res, &idx) != TCL_OK) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf("Error ! index is not valid."));
	    return TCL_ERROR;
	}
	/* index starts at 0 ! */
	idx -= 1;
	if (idx >= bioPtr->nbSeqs) {
	    idx = bioPtr->nbSeqs - 1;
	}
	if (idx < 0) {
	    idx = 0;
	}
    }

#ifdef DEBUG_PASTECMD
    fprintf(stderr, "\n== in biotextPasteCmd =========\n");
    fprintf(stderr, "widget %s\n", bioPtr->widgetName);
    fflush(stderr);
#endif
  
    /*
     * First insert empty rows ...
     */
    //DumpCoordsTag(bioPtr, "PasteCmd\nAVANT InsertRows");
    BiotextInsertRows(bioPtr, idx, bioPtr->nbSeqsCache);
    //DumpCoordsTag(bioPtr, "PasteCmd\nAPRES InsertRows");
    //DumpSeqs(bioPtr,"PasteCmd\napres insertrows");
    
    /* Check if length of sequences to be inserted is 
       the same of the one already present
    */
    diff = bioPtr->lgSeqs - bioPtr->lgSeqsCache;
#ifdef DEBUG_PASTECMD
    fprintf(stderr, "len seq wdg %d cache %d diff= %d\n", bioPtr->lgSeqs, bioPtr->lgSeqsCache, diff);
    fflush(stderr);
#endif
    if (diff > 0) {
	/* we must add cols to buffer seqs */
	BiotextAddColsToBufferSeqs(bioPtr, diff);
    } else if (diff < 0) {
	/* we must add cols to present seqs */
	diff = abs(diff);
	BiotextInsertCols(bioPtr, bioPtr->lgSeqs-1, diff, -1);
    }
  
    /* Fill newly inserted rows */
    for (i=idx+1,k=bioPtr->nbSeqsCache-1; k>=0; i++,k--) {
	bioPtr->SeqGrp[i] = bioPtr->SeqGrpCache[k];
	strcpy(bioPtr->SeqMat[i], bioPtr->SeqMatCache[k]);
	if (bioPtr->nbMapping > 0) {
	    for (j=0; j<bioPtr->lgSeqs; j++) {
		bioPtr->MapSeq[i][j] = bioPtr->MapSeqCache[k][j];
	    }
	}
    }

    //DumpCoordsTag(bioPtr, "PasteCmd\nBefore treating tags ....");
    /* Treat tags only if tags exist */
    if (bioPtr->Tags == NULL || bioPtr->TagsCache == NULL) {
	goto done;
    }

    int nelt, iTag, iTagCache, len;
    Tcl_Size neltSz, lenSz;
    char *tagName;
    Tcl_Obj *tagObj, *tmpObj, *tmpObj2, **tmpObj3;
    Tcl_Obj **eltObj;

    for(i=0; i<bioPtr->nbTagsCache; i++) {
	/* check if cached tag still exists */
	Tcl_ListObjIndex(interp, bioPtr->listTagsCache, i, &tagObj);
	Tcl_GetIntFromObj(interp, tagObj, &iTagCache);
	tagName = bioPtr->TagsCache[iTagCache].tagName;
	iTag = GetIndexOfTag(bioPtr, tagName, 1);
	if (iTag == -1) {
	    /* tag does not exists anymore */
	    continue;
	}
    
	Tcl_ListObjGetElements(interp, bioPtr->TagsCache[iTagCache].coords, &neltSz, &eltObj);
	nelt = (int)neltSz;
	tmpObj = Tcl_NewListObj(0, NULL);
	for (j=nelt-1; j>=0; j--) {
	    Tcl_ListObjAppendElement(interp, tmpObj, Tcl_DuplicateObj(eltObj[j]));
	}
	Tcl_ListObjGetElements(interp, tmpObj, &neltSz, &tmpObj3);
	nelt = (int)neltSz;
    
	tmpObj2 = Tcl_DuplicateObj(bioPtr->Tags[iTag].coords);
	Tcl_ListObjLength(interp, tmpObj2, &lenSz);
	len = (int)lenSz;
#ifdef DEBUG_PASTECMD
	fprintf(stderr, "%s : iTag %d\nnbr Coords before %d\n", tagName, iTag, len);
#endif
	Tcl_ListObjReplace(interp, tmpObj2, idx+1, nelt, nelt, tmpObj3);
	bioPtr->Tags[iTag].coords = Tcl_DuplicateObj(tmpObj2);
	Tcl_ListObjLength(interp, bioPtr->Tags[iTag].coords, &lenSz);
	len = (int)lenSz;
#ifdef DEBUG_PASTECMD
	fprintf(stderr, "nbr Coords after  %d\n",len);
	fflush(stderr);
#endif
    }
    //DumpCoordsTag(bioPtr, "PasteCmd\nAFTER treat tags");
    //DumpSeqs(bioPtr, "PasteCmd\nafter treat tags");
 done:  
    /* Indicate there's nothing more to paste */
    //    BiotextInitBuffer(bioPtr, 0);

    bioPtr->flags |= UPDATE_SCROLLBAR;
    BiotextEventuallyRedraw(bioPtr);

    return TCL_OK;
}

/* ==== BiotextGetIndex (from line 5361) ==== */
static int
BiotextGetIndex(register Biotext *bioPtr, 
		char *str, 
		int *row_p, 
		int *col_p) 
{
    int r, c;
    char dummy;
  
    //fprintf(stderr,"\nGetIndex:\nEnter with >%s<\n",str);
    //fflush(stderr);
    /* Bug fix: on an empty widget (nbSeqs==0 or lgSeqs==0 -- e.g.
     * before any sequence has ever been loaded), the clamping code
     * below ("if (r >= bioPtr->nbSeqs) r = bioPtr->nbSeqs - 1;") used
     * to silently produce r=-1/c=-1 while still returning TCL_OK, as
     * if the index had been resolved successfully. Every caller of
     * BiotextGetIndex() uses that -1 as a live array index afterward
     * (bioPtr->SeqMat[r], bioPtr->SeqGrp[r], ...) -- confirmed by a
     * real SIGSEGV in BiotextDelcharsCmd ("nGrp = bioPtr->SeqGrp[r];"
     * with r=-1) when reproducing this under gdb. Reject any index on
     * an empty widget up front instead of returning a bogus "success". */
    if (bioPtr->nbSeqs <= 0 || bioPtr->lgSeqs <= 0) {
	Tcl_AppendStringsToObj(Tcl_GetObjResult(bioPtr->interp),
	    "bad index \"", str, "\": biotext widget is empty", (char *)NULL);
	return TCL_ERROR;
    }
    if (strcmp(str,"end") == 0) {
	r = bioPtr->nbSeqs - 1;
	c = bioPtr->lgSeqs - 1;
    } else if (str[0] == '@') {
	/* @x,y : pixel coordinate 
	 * index is a pixel position */
	//fprintf(stderr, "index= %s\n", str);
	//fflush(stderr);
	int x, y;
	if (sscanf(str+1, "%d,%d%c", &x, &y, &dummy) != 2) {
	    goto IndexError;
	}
	Biotext_Pixels2Coords(bioPtr, x, y, &c, &r);
	//rR fprintf(stderr,"GetIndex %s in pxl -> r= %d c= %d\n", str, r, c);
	//rR fflush(stderr);
    } else {
	/* index is row.col or row.end */
	if (sscanf(str,"%d.%d%c", &r, &c, &dummy) != 2) {
	    const char *start;
	    char *end;
	    start = str;
	    r = strtol(start, &end, 0);
	    if ((start == end) || (*end != '.')) {
		goto IndexError;
	    }
	    start = end+1;
	    if (strcmp(start,"end") == 0) {
		c = bioPtr->lgSeqs-1;
	    } else {
		goto IndexError;
	    }
	}
	r -= 1;
    }
    //fprintf(stderr,"  r= %d c= %d\n",r, c);
    //fflush(stderr);
    /* in the line.col form, line starts at 1 */
    //r -= 1;
  
    /* check for dummies values */
    if (r <  0)
	r = 0;
    if (r >= bioPtr->nbSeqs)
	r = bioPtr->nbSeqs - 1;
    if (c <  0)
	c = 0;
    if (c >= bioPtr->lgSeqs)
	c = bioPtr->lgSeqs - 1;
  
    if (row_p)
	*row_p = r;
    if (col_p)
	*col_p = c;

    return TCL_OK;
  
 IndexError:
    Tcl_AppendStringsToObj(Tcl_GetObjResult(bioPtr->interp), "bad index \"", str, "\": must be end, current, @x,y or <row>.<col>", (char *)NULL);
    return TCL_ERROR;
}

/* ==== BiotextInitBuffer (from line 5954) ==== */
static int
BiotextInitBuffer(register Biotext *bioPtr,
		  int nseq) 
{
    Tcl_Interp *interp = bioPtr->interp;
    int i, k, iTag, len;
    Tcl_Obj *tmpObj, *tagObj;
    
    /* cleanup if necessary */
    if (bioPtr->nbSeqsCache > 0) {
	ckfree((char *)bioPtr->SeqGrpCache);
	for (i=0; i<bioPtr->nbSeqsCache; i++) {
	    ckfree((char *)bioPtr->SeqMatCache[i]);
	    if (bioPtr->MapSeqCache != NULL) {
		ckfree((char *)bioPtr->MapSeqCache[i]);
	    }
	}
	ckfree((char *)bioPtr->SeqMatCache);
	/* Bug fix: was gated on bioPtr->nbMapping (current state) instead
	 * of bioPtr->MapSeqCache != NULL (whether a cache array actually
	 * exists). If mapping was later disabled, the outer MapSeqCache
	 * pointer-array allocation would leak here even though its rows
	 * were freed above. Matches the guard already used consistently
	 * in the widget destroy cleanup path. */
	if (bioPtr->MapSeqCache != NULL) {
	    ckfree((char *)bioPtr->MapSeqCache);
	}
    }
    bioPtr->nbSeqsCache = 0;
    bioPtr->SeqGrpCache = NULL;
    bioPtr->SeqMatCache = NULL;
    bioPtr->MapSeqCache = NULL;
    
    /* tags */
    if (bioPtr->nbTagsCache > 0) {
	for (i=0; i<bioPtr->nbTagsCache; i++) { 
	    ckfree((char *)bioPtr->TagsCache[i].tagName);
	}
	ckfree((char *)bioPtr->TagsCache);
    }
    bioPtr->listTagsCache = NULL;
    bioPtr->nbTagsCache = 0;
    bioPtr->TagsCache = NULL;
    
    if (nseq == 0) {
	return TCL_OK;
    }

    /* Do the allocations */
    bioPtr->SeqGrpCache = (int *)ckalloc(nseq*sizeof(int));
    bioPtr->SeqMatCache = (char **)ckalloc(nseq*sizeof(char *));
    if (bioPtr->nbMapping > 0) {
	bioPtr->MapSeqCache = (int **)ckalloc(nseq*sizeof(int *));
    }
    for (i=0; i<nseq; i++) {
	bioPtr->SeqMatCache[i] = (char *)ckalloc((bioPtr->lgSeqs+1)*sizeof(char));
	if (bioPtr->nbMapping > 0) {
	    bioPtr->MapSeqCache[i] = (int *)ckalloc(bioPtr->lgSeqs*sizeof(int));
	}
    }
    bioPtr->nbSeqsCache = nseq;
    bioPtr->lgSeqsCache = bioPtr->lgSeqs;

    if(bioPtr->Tags != NULL) {
	bioPtr->listTagsCache = Tcl_DuplicateObj(bioPtr->listTags);
	bioPtr->nbTagsCache = bioPtr->nbTags;
	bioPtr->TagsCache = (Tag *)ckalloc(bioPtr->nbTags*sizeof(Tag));

	tmpObj = Tcl_NewListObj(0, NULL);
	for(k=0; k<nseq; k++) {
	    Tcl_ListObjAppendElement(interp, tmpObj, Tcl_NewListObj(0, NULL));
	}
	for (i=0; i<bioPtr->nbTags; i++) {
	    Tcl_ListObjIndex(interp, bioPtr->listTagsCache, i, &tagObj);
	    Tcl_GetIntFromObj(interp, tagObj, &iTag);
	    bioPtr->TagsCache[iTag].bg = bioPtr->Tags[iTag].bg;
	    bioPtr->TagsCache[iTag].fg = bioPtr->Tags[iTag].fg;

	    bioPtr->TagsCache[iTag].tagName = '\0';
	    len = strlen(bioPtr->Tags[iTag].tagName);
	    if (len >0) {
		bioPtr->TagsCache[iTag].tagName = (char *)ckalloc((len+1)*sizeof(char));
		strcpy(bioPtr->TagsCache[iTag].tagName, bioPtr->Tags[iTag].tagName);
	    }
#ifdef USE_XFT
	    bioPtr->TagsCache[iTag].XftCfg = bioPtr->Tags[iTag].XftCfg;
	    bioPtr->TagsCache[iTag].XftCbg = bioPtr->Tags[iTag].XftCbg;
#endif
	    bioPtr->TagsCache[iTag].fgBorder = bioPtr->Tags[iTag].fgBorder;
	    bioPtr->TagsCache[iTag].bgBorder = bioPtr->Tags[iTag].bgBorder;
	    bioPtr->TagsCache[iTag].Cfg = bioPtr->Tags[iTag].Cfg;
	    bioPtr->TagsCache[iTag].Cbg = bioPtr->Tags[iTag].Cbg;
	    bioPtr->TagsCache[iTag].coords = Tcl_DuplicateObj(tmpObj);
	    bioPtr->TagsCache[iTag].nbCoords = bioPtr->Tags[iTag].nbCoords;
	}
    }
    //DumpCoordsTag(bioPtr, "Fin initbuffer");
    return TCL_OK;
}

/* ==== BiotextCopySeqsToBuffer (from line 6070) ==== */
static int
BiotextCopySeqsToBuffer(register Biotext *bioPtr,
			int nseqs,
			Tcl_Obj **OIdx) 
{  
    Tcl_Interp *interp = bioPtr->interp;
    int i, j, k, idx, iTag;
    Tcl_Obj *coordsObj, *tmpObj, *tagObj;
  
    bioPtr->nbSeqsCache = nseqs;
    for (i=0; i<nseqs; i++) {
	Tcl_GetIntFromObj(interp, OIdx[i], &idx);
	bioPtr->SeqGrpCache[i] = bioPtr->SeqGrp[idx];
	strcpy(bioPtr->SeqMatCache[i], bioPtr->SeqMat[idx]);
	if (bioPtr->nbMapping > 0) {
	    for (j=0; j<bioPtr->lgSeqs; j++) {
		bioPtr->MapSeqCache[i][j] = bioPtr->MapSeq[idx][j];
	    }
	}
    }

    if (bioPtr->TagsCache != NULL) {
	bioPtr->nbTagsCache = bioPtr->nbTags;
	bioPtr->listTagsCache = Tcl_DuplicateObj(bioPtr->listTags);
	for (k=0; k<bioPtr->nbTagsCache; k++) {
	    Tcl_ListObjIndex(interp, bioPtr->listTagsCache, k, &tagObj);
	    Tcl_GetIntFromObj(interp, tagObj, &iTag);
	    
	    coordsObj = Tcl_NewListObj(0, NULL);
	    for (i=0; i<nseqs; i++) {
		Tcl_GetIntFromObj(interp, OIdx[i], &idx);
		Tcl_ListObjIndex(interp, bioPtr->Tags[iTag].coords, idx, &tmpObj);
		Tcl_ListObjAppendElement(interp, coordsObj, Tcl_DuplicateObj(tmpObj));
	    }
	    bioPtr->TagsCache[iTag].coords = Tcl_DuplicateObj(coordsObj);

	}
    }
    fflush(stderr);
    //DumpCoordsTag(bioPtr, "End of CopySeqsToBuffer\n");
    return TCL_OK;
}

/* ==== BiotextAddColsToBufferSeqs (from line 6128) ==== */
static int
BiotextAddColsToBufferSeqs(register Biotext *bioPtr,
			   int ncols) 
{
    int i, j, itag;

    /* re-alloc matrices to the actual sequence length */
    for (i=0;i<bioPtr->nbSeqsCache;i++) {
	bioPtr->SeqMatCache[i] = (char *)ckrealloc((char *)bioPtr->SeqMatCache[i],(bioPtr->lgSeqsCache+ncols+1)*sizeof(char));
	/* Bug fix: bioPtr->MapSeqCache (the outer int** array) is only
	 * allocated in BiotextInitBuffer when bioPtr->nbMapping > 0; it
	 * stays NULL otherwise. Indexing it unconditionally here caused
	 * a NULL-pointer dereference (crash) as soon as a paste needed
	 * to widen the buffered sequences while no mapping was active. */
	if (bioPtr->nbMapping > 0) {
	    bioPtr->MapSeqCache[i] = (int *)ckrealloc((char *)bioPtr->MapSeqCache[i],(bioPtr->lgSeqsCache+ncols)*sizeof(int));
	}
	//TODO tags
    }
  
    /* search for gap tag index */
    itag = GetIndexOfTag(bioPtr, ".", 0);

    /* fill added columns with gaps */
    for(i=0;i<bioPtr->nbSeqsCache;i++) {
	for(j=bioPtr->lgSeqsCache;j<bioPtr->lgSeqsCache+ncols;j++) {
	    bioPtr->SeqMatCache[i][j] = '.';
	    if (bioPtr->nbMapping > 0) {
		bioPtr->MapSeqCache[i][j] = itag;
	    }
	    //TODO tags
	}
	bioPtr->SeqMatCache[i][bioPtr->lgSeqsCache+ncols] = '\0';
    }

    bioPtr->lgSeqsCache = bioPtr->lgSeqs;

    return TCL_OK;
}

/* ==== GetIndexOfTag (from line 6239) ==== */
static int
GetIndexOfTag(register Biotext *bioPtr,
	      char *tagName,
	      int mapOrtag) /* 0 if map, 1 if tag */ 
{
    int i, iTag, nelt;
  
    if (! mapOrtag) {
	nelt = bioPtr->nbMapping;
    } else {
	nelt = bioPtr->nbTags;
    }
  
    iTag = -1;

#ifdef DEBUG_GIOT
    fprintf(stderr,"\n== in GetIndexOfTag==========\n");
    fprintf(stderr, "widget %s\n", bioPtr->widgetName);
    fprintf(stderr, " nb elt %d mapOrtag %d\n", nelt, mapOrtag);
    fflush(stderr);
#endif  
    for (i=0; i<nelt; i++) {
	if (! mapOrtag) {
#ifdef DEBUG_GIOT
	    fprintf(stderr, ">%c< >%c< %d %d\n", tagName[0], bioPtr->Mapping[i].smap, tagName[0], bioPtr->Mapping[i].smap);
	    fflush(stderr);
#endif
	    if (tagName[0] == bioPtr->Mapping[i].smap) {
		iTag = i;
		break;
	    }
	} else {
#ifdef DEBUG_GIOT
	    fprintf(stderr, "tag >%s< = >%s<\n", tagName, bioPtr->Tags[i].tagName);
	    fflush(stderr);
#endif
	    if (strcmp(tagName, bioPtr->Tags[i].tagName) == 0) {
		iTag = i;
		break;
	    }
	}
    }
  
    return iTag;
}

/* ==== BiotextInsertGapsInTags (from line 6370) ==== */
static int
BiotextInsertGapsInTags(register Biotext *bioPtr,
			char *index,
			int count) 
{
    Tcl_Interp *interp = bioPtr->interp;
    int i, k, start, stop, nval, row, col, right, nrow;
    Tcl_Size nrowSz, nvalSz;
    Tcl_Obj *tmpObj, *coordsObj, *newObj, **valObj, **rowObj;

    if (BiotextGetIndex(bioPtr, index, &row, &col) != TCL_OK) {
	return TCL_ERROR;
    }

    if (count < 0) {
	count = abs(count);
	right = 0;
    } else {
	right = 1;
    }

    for (i=0; i<bioPtr->nbTags; i++) {
	coordsObj = Tcl_DuplicateObj(bioPtr->Tags[i].coords);
	Tcl_ListObjGetElements(interp, coordsObj, &nrowSz, &rowObj);
	nrow = (int)nrowSz;
	//Tcl_ListObjIndex(interp, coordsObj, row, &tmpObj);
	tmpObj = Tcl_NewListObj(0, NULL);
	tmpObj = Tcl_DuplicateObj(rowObj[row]);
	//valObj = Tcl_NewListObj(0, NULL); 
	Tcl_ListObjGetElements(interp, tmpObj, &nvalSz, &valObj);
	nval = (int)nvalSz;
	newObj = Tcl_NewListObj(0, NULL);
	for (k=0; k<nval; k+=2) {
	    Tcl_GetIntFromObj(interp, valObj[k], &start);
	    Tcl_GetIntFromObj(interp, valObj[k+1], &stop);

	    if (right) {
		if (stop > col) {
		    stop += count;
		    if (start >= col) {
			start += count;
		    }
		}
	    } else {
		if (stop >= col) {
		    stop -= count;
		    if (start >= col) {
			start -= count;
		    }
		}
	    }
	    Tcl_ListObjAppendElement(interp, newObj, Tcl_NewIntObj(start));
	    Tcl_ListObjAppendElement(interp, newObj, Tcl_NewIntObj(stop));

	    /* Bug fix: valObj[k]/valObj[k+1] are borrowed pointers into
	     * tmpObj's internal list representation (from
	     * Tcl_ListObjGetElements); this function never took its own
	     * reference on them. Manually decrementing their refcount here
	     * can drop a shared Tcl_Obj to zero references while tmpObj
	     * (and possibly the original, not-yet-replaced coords list,
	     * which may share the same underlying int objects) still
	     * points to it, causing a use-after-free / heap corruption.
	     * Removed: there is nothing to release here, since these
	     * objects are owned by the list, not by this function. */
	}
	rowObj[row] = Tcl_DuplicateObj(newObj);
	bioPtr->Tags[i].coords = Tcl_NewListObj(0, NULL);
	for (k=0; k<nrow; k++) {
	    Tcl_ListObjAppendElement(interp, bioPtr->Tags[i].coords, Tcl_DuplicateObj(rowObj[k]));
	    /* Bug fix: same issue as the valObj[k]/valObj[k+1] case above.
	     * rowObj[k] is a borrowed pointer into coordsObj's internal
	     * list representation (from Tcl_ListObjGetElements); this
	     * function took no reference of its own on it (it duplicates
	     * it just above before appending). Decrementing it here can
	     * free a Tcl_Obj that coordsObj still accounts for, causing
	     * use-after-free / heap corruption. Removed. */
	}
    }
    //DumpCoordsTag(bioPtr, "sort insert gap in tags");
    return TCL_OK;
}
