/*
 *
 *
 * tkBiotext.c --
 *
 *	This module implements "Biotext" widgets that are object based. A
 *	"Biotext" is a widget that displays a single Biotext that can be moved
 *	around and resized. This file is intended as an example of how to
 *	build a widget; it isn't included in the normal wish, but it is
 *	included in "tktest".
 *
 * Copyright (c) Luc Moulinier
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version. See the file "LICENSE" for details.
 *
 * RCS: @(#) $Id: tkBiotext.c,v 1.13 2008/10/17 23:18:37 nijtmans Exp $
 */

#include "tkBiotext.h"

//#define DEF_TEXT_FONT "TkFixedFont"
#define DEF_TEXT_FONT "Courier-24:normal"

#ifndef DLLEXPORT
#define DLLEXPORT __declspec(dllexport)
#endif 

#ifdef DEBUG
#define DEBUG 0
#define DEBUG_CMD 1 /* which cmd is invoked for the widget ? */
#define DEBUG_ADDSEQS 1 /* BiotextAddSeqs command */
#define DEBUG_CONFIGNOTIFY 1 /* !! Not a function, but event ConfifureNotify */
#define DEBUG_OUTPUTCMD 1 /* BiotextOutputCmd */
#define DEBUG_BIOTEXTCMD 1 /* BiotextObjCmd */
#define DEBUG_CONFIGURECMD 1 /* BiotextConfigureCmd */
#defined DEBUG_WORLDCHANGED 1 /* BiotextWorldChangedCmd */
#define DEBUG_TAGREMOVE 1
#define DEBUG_TAGCMD 1 /* BiotextTagCmd */
#define DEBUG_CURSORCMD 1 /* BiotextCursorCmd */
#define DEBUG_INSERTCOLS 1 /* BiotextInsertCols */
#define DEBUG_SEECMD 1 /* biotextSeeCmd */
#define DEBUG_TAGCONFIDCMD 1 /* BiotextTagConfigSubCmd */
#define DEBUG_TAGADDCMDCMD 1 /* BiotextTagAddSubCmd */
#define DEBUG_TAGDELETECMD 1 /* BiotextTagDeleteSubCmd */
#define DEBUG_INSERTROW 1 /* BiotextInsertRow */
#define DEBUG_GIOT 1 /* GetIndexOfTag */
#define DEBUG_PASTECMD 1 /* BiotextPasteCmd */
#define DEBUG_SEQUENCESCMD 1 /* BiotextSequencesCmd */
#define DEBUG_YVIEW 1 /* BiotextYViewCmd + ChangeBiotextYView */
#define DEBUG_DELCOLS 1 /* BiotextDeleteCols helper function */
#endif
//#define DEBUG_CMD 1
//#define DEBUG_TAGCONFIGCMD 1
//#define DEBUG_TAGCONFIDCMD 1 /* BiotextTagConfigSubCmd */
//#define DEBUG_TAGREMOVE 1
//#define DEBUG_TAGADDCMD 1

#ifdef USE_XFT
#define WITHXFT 1
#endif

static const Tk_ClassProcs BiotextClass = {
    sizeof(Tk_ClassProcs),	/* size */
    BiotextWorldChanged,		/* worldChangedProc */
    NULL,
    NULL
};

/*
 * Information used for argv parsing.
 */
 
static const Tk_OptionSpec BiotextOptionSpecs[] = {
    {TK_OPTION_BORDER, "-background", "background", "Background",
     "black", -1, Tk_Offset(Biotext, bgBorder),
     0, (ClientData) "#d9d9d9", 0},
    {TK_OPTION_SYNONYM, "-bd", NULL, NULL,
     NULL, 0, 0, 0, (ClientData) "-borderwidth", 0},
    {TK_OPTION_SYNONYM, "-bg", NULL, NULL,
     NULL, 0, 0, 0, (ClientData) "-background", 0},
    {TK_OPTION_PIXELS, "-borderwidth", "borderWidth", "BorderWidth",
     "1", -1, Tk_Offset(Biotext, borderWidth),
     TK_OPTION_NULL_OK, NULL, 0},
    {TK_OPTION_STRING, "-class", "class", "Class",
     "Biotext", -1, Tk_Offset(Biotext, className),
     0, NULL, 0},
    {TK_OPTION_COLOR, "-cursorcolor", NULL, NULL,
     "yellow", -1, Tk_Offset(Biotext, cursorColor),
     TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_BOOLEAN, "-dbl", "doubleBuffer", "DoubleBuffer",
     "1", -1, Tk_Offset(Biotext, doubleBuffer),
     0, 0, 0},
    {TK_OPTION_FONT, "-font", "font", "Font",
     DEF_BIOTEXT_FONT, -1, Tk_Offset(Biotext, tkfont), 0, 0, 0},
    {TK_OPTION_SYNONYM, "-fg", NULL, NULL,
     NULL, 0, 0, 0, (ClientData) "-foreground", 0},
    {TK_OPTION_BORDER, "-foreground", "foreground", "Foreground",
     "white", -1, Tk_Offset(Biotext, fgBorder),
     0, (ClientData) "white", 0},
    {TK_OPTION_INT, "-height", "height", "Height",
     "20", -1, Tk_Offset(Biotext, heightChar),
     0, 0, 0},
    {TK_OPTION_INT, "-insertofftime", "insertOffTime", "OffTime",
     "300", -1, Tk_Offset(Biotext, insertOffTime),
     0, 0, 0},
    {TK_OPTION_INT, "-insertontime", "insertOnTime", "OnTime",
     "600", -1, Tk_Offset(Biotext, insertOnTime),
     0, 0, 0},
    {TK_OPTION_PIXELS, "-highlightthickness", "highlightThickness",
     "HighlightThickness", "1", -1,
     Tk_Offset(Biotext, highlightWidth), 0, 0, 0},
    {TK_OPTION_RELIEF, "-relief", "relief", "Relief",
     "raised", -1, Tk_Offset(Biotext, relief),
     TK_OPTION_NULL_OK, NULL, 0},
    {TK_OPTION_BOOLEAN, "-setgrid", "setGrid", "SetGrid",
     "1", -1, Tk_Offset(Biotext, setGrid), 
     0, 0, 0},
    {TK_OPTION_STRING_TABLE, "-state", "state", "State",
     "normal", -1, Tk_Offset(Biotext, state),
     0, (ClientData) stateStrings, 0},
    {TK_OPTION_INT, "-width", "width", "Width",
     "60", -1, Tk_Offset(Biotext, widthChar),
     0, 0, 0},
    {TK_OPTION_STRING, "-xscrollcommand", "xScrollCommand", "ScrollCommand",
     "", -1, Tk_Offset(Biotext, xscrollCmd),
     TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_STRING, "-yscrollcommand", "yScrollCommand", "ScrollCommand",
     "", -1, Tk_Offset(Biotext, yscrollCmd),
     TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_END, NULL, NULL, NULL, NULL, 0, 0, 0, 0, 0}
};

/*
 *--------------------------------------------------------------
 *
 * BiotextObjCmd --
 *
 *	This procedure is invoked to process the 
 *      "Biotext" Tcl command. It
 *	creates a new "Biotext" widget.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	A new widget is created and configured.
 *
 *--------------------------------------------------------------
 */
static int
BiotextObjCmd(ClientData clientData,
	      Tcl_Interp *interp,
	      int objc,
	      Tcl_Obj *const objv[]) 
{
    register Biotext *bioPtr;
    Tk_Window tkwin;
    Tk_OptionTable optionTable;
  
    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "pathName ?-option value ...?");
	return TCL_ERROR;
    }
  
    tkwin = Tk_CreateWindowFromPath(interp, Tk_MainWindow(interp), Tcl_GetString(objv[1]), NULL);
    if (tkwin == NULL) {
	return TCL_ERROR;
    }
  
    /*
     * Create the option table for this widget class.
     * If it has already been
     * created, the refcount will get bumped and j
     * ust the pointer will be
     * returned. The refcount getting bumped does
     * not concern us, because Tk
     * will ensure the table is deleted when the 
     * interpreter is destroyed.
     */
    optionTable = Tk_CreateOptionTable(interp, BiotextOptionSpecs);
    if (optionTable == NULL) {
	return TCL_ERROR;
    }
  
    /*
     * Initialize the widget record. 
     * The memset allows us to set
     * just the non-NULL/0 items.
     */
    bioPtr = (Biotext *)ckalloc(sizeof(Biotext));
    memset(bioPtr, 0, sizeof(Biotext));
  
    /* ===========================================
     * Initialize element of the Biotext structure
     */
    bioPtr->interp         = interp;
    /* Bug fix: this used to be a fixed 1000-byte buffer with an
     * unbounded strcpy() into it. Any widget path name of 1000+
     * characters (deeply nested Tk paths, generated names, etc.)
     * would overflow this heap buffer. Size it to the actual path
     * name length instead. */
    bioPtr->widgetName = (char *)ckalloc((strlen(Tcl_GetString(objv[1]))+1)*sizeof(char));
    strcpy(bioPtr->widgetName, Tcl_GetString(objv[1]));
    bioPtr->tkwin          = tkwin;
    bioPtr->display        = Tk_Display(tkwin);
    bioPtr->screen         = Tk_Screen(tkwin);
    bioPtr->widgetCmd      = Tcl_CreateObjCommand(interp, Tk_PathName(tkwin), BiotextWidgetObjCmd, bioPtr, (Tcl_CmdDeleteProc *)BiotextDeletedProc);
  
    bioPtr->gc             = None;
    bioPtr->optionTable    = optionTable;

    /* widget config */
    bioPtr->flags = 0;
    bioPtr->state          = STATE_NORMAL;
    bioPtr->doubleBuffer   = 0;
    bioPtr->reliefPtr      = NULL;
    bioPtr->relief         = TK_RELIEF_RAISED;
    bioPtr->borderWidthPtr = NULL;
    bioPtr->selBorderWidth = 0;
    bioPtr->inset          = 0;

    /* widget dimensions */
    bioPtr->width          = -1;
    bioPtr->height         = -1;
    bioPtr->widthChar      = 60;
    bioPtr->heightChar     = 20;
    bioPtr->fullCols = -1;
    bioPtr->fullRows = -1;

    /* widget content */
    bioPtr->leftIndex      = 0;
    bioPtr->topIndex       = 0;
    bioPtr->currentR       = -1;
    bioPtr->currentC       = -1;
    bioPtr->SeqMat         = NULL;
    bioPtr->nbSeqs         = 0;

    /* mapping */
    bioPtr->SeqGrp         = NULL;
    bioPtr->MapSeq         = NULL;
    bioPtr->Mapping        = NULL;
    bioPtr->nbMapping      = 0;

    /* tagging */
    bioPtr->Tags          = NULL;
    bioPtr->nbTags        = 0;
    bioPtr->nbTagsAlloc   = 0;
    bioPtr->listTags      = Tcl_NewListObj(0, NULL);
    bioPtr->Lock          = 1;

    /* Set font, get font characteristics */
#ifdef USE_XFT
    bioPtr->xft            = 1;
#else
    bioPtr->xft            = 0;
#endif
    bioPtr->FontXft         = NULL;
    bioPtr->FontName        = NULL;
    bioPtr->FontXlfd = NULL;
    bioPtr->FontXft = NULL;
#if USE_XFT
    bioPtr->xftFont = NULL;
#endif
    bioPtr->FontFamily = NULL;
    bioPtr->FontFamily = (char *)ckalloc((strlen("Courier")+1)*sizeof(char));
    strcpy(bioPtr->FontFamily, "Courier");
    bioPtr->FontSize        = 12;
    bioPtr->FontWeight = NULL;
    bioPtr->FontWeight = (char *)ckalloc((strlen("normal")+1)*sizeof(char));
    strcpy(bioPtr->FontWeight, "normal");
    bioPtr->charWidth       = 1;
    bioPtr->charHeight      = 1;
    /* *compute values according to font */
    BiotextSetFont(bioPtr);

    /* Group initialisation */
    bioPtr->nbrGrps         = 0;
    bioPtr->GroupList       = NULL;

    /* Cut and Paste initialisation */
    bioPtr->nbSeqsCache     = 0;
    bioPtr->SeqGrpCache     = NULL;
    bioPtr->SeqMatCache     = NULL;
    bioPtr->MapSeqCache     = NULL;
    bioPtr->listTagsCache   = NULL;
    bioPtr->nbTagsCache     = 0;
    bioPtr->TagsCache       = NULL;

    /* Bug fix: this used to be "Tcl_Preserve(bioPtr->tkwin);". Every
     * other Tcl_Preserve/Tcl_Release pair in this file operates on
     * "bioPtr" (the ClientData widget record) -- never on tkwin, a
     * Tk_Window -- and there is no matching Tcl_Release(bioPtr->tkwin)
     * anywhere in the file. As written, this permanently leaked one
     * entry in Tcl's internal preserve/release table per widget
     * instance created (Tcl_Preserve records are only reclaimed by a
     * matching Tcl_Release). It also did not protect bioPtr itself:
     * every function below that can trigger destruction mid-call
     * (BiotextDisplay, BiotextObjEventProc, BiotextWidgetObjCmd, ...)
     * already brackets its own body with Tcl_Preserve(bioPtr) /
     * Tcl_Release(bioPtr), so this stray call served no purpose.
     * Removed rather than "corrected" to Tcl_Preserve(bioPtr), since
     * there is no natural matching Tcl_Release point in the
     * constructor either -- the existing per-function preserve/release
     * pairs are already sufficient to protect bioPtr during callbacks. */

    Tk_SetClass(tkwin, "Biotext");
    Tk_SetClassProcs(bioPtr->tkwin, &BiotextClass, bioPtr);
  
    /*
     * set up event proc
     */
    Tk_CreateEventHandler(bioPtr->tkwin,
			  ExposureMask|StructureNotifyMask|FocusChangeMask,
			  BiotextObjEventProc, 
			  bioPtr);
  
    if (Tk_InitOptions(interp, (char *) bioPtr, optionTable, tkwin) != TCL_OK) {
	Tk_DestroyWindow(bioPtr->tkwin);
    
	return TCL_ERROR;
    }
  
    if (BiotextConfigure(interp, bioPtr, objc-2, objv+2) != TCL_OK) {
	Tk_DestroyWindow(bioPtr->tkwin);
    
	return TCL_ERROR;
    }
#ifdef DEBUG_BIOTEXTCMD
    fprintf(stderr,"\n== in BiotextObjCmd -----------------\n");
    fprintf(stderr,"widget %s\n", bioPtr->widgetName);
    fprintf(stderr,"widthChar %d heightChar %d", bioPtr->widthChar, bioPtr->heightChar);
    fprintf(stderr,"\n------------------\n");
    fflush(stderr);
#endif
  
    Tcl_ResetResult(interp);
    Tcl_SetObjResult(interp, Tcl_NewStringObj((char *)Tk_PathName(tkwin), -1));
  
    return TCL_OK;
}


/*
 *--------------------------------------------------------------
 *
 * BiotextWidgetObjCmd --
 *
 *	This procedure is invoked to process the Tcl command that corresponds
 *	to a widget managed by this module. See the user documentation for
 *	details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *--------------------------------------------------------------
 */

static int
BiotextWidgetObjCmd(ClientData clientData,
		    Tcl_Interp *interp,
		    int objc,
		    Tcl_Obj * const objv[]) 
{
    register Biotext *bioPtr = (Biotext *)clientData;
    int index, result = TCL_OK;
    Tcl_Obj *resultObjPtr;

#ifdef DEBUG_CMD
    if (objc > 2) {
	fprintf(stderr, "\n%s cmd %s %s\n", bioPtr->widgetName, Tcl_GetString(objv[1]), Tcl_GetString(objv[2]));
    } else {
	fprintf(stderr, "\n%s cmd %s\n", bioPtr->widgetName, Tcl_GetString(objv[1]));
    }
    fflush(stderr);
#endif
	  
    static CONST84 char *BiotextOptions[] = {"addseqs", "cget", "chars", "clean", "configure", "copy", "cursor", "cut", "delchars", "delete", "font", "group",  "index", "insert", "lock", "map", "mapping", "tag", "output", "paste", "push", "see", "sequences", "ungroup", "xview", "yview", NULL};
    enum selOptions {
	Biotext_ADDSEQS, Biotext_CGET, Biotext_CHARS, Biotext_CLEAN, Biotext_CONFIGURE, Biotext_COPY, Biotext_CURSOR, Biotext_CUT, Biotext_DELCHARS, Biotext_DELETE, Biotext_FONT, Biotext_GROUP, Biotext_INDEX, Biotext_INSERT, Biotext_LOCK, Biotext_MAP, Biotext_MAPPING, Biotext_TAG, Biotext_OUTPUT, Biotext_PASTE, Biotext_PUSH, Biotext_SEE, Biotext_SEQUENCES, Biotext_UNGROUP, Biotext_XVIEW, Biotext_YVIEW,
    };
  
    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "option ?arg arg...?");
	return TCL_ERROR;
    }
  
    if (Tcl_GetIndexFromObj(interp, objv[1], BiotextOptions, "command", 0, &index) != TCL_OK) {
	return TCL_ERROR;
    }
  
    Tcl_Preserve((ClientData) bioPtr);

    switch ((enum selOptions) index) {
    case Biotext_ADDSEQS:
	if (BiotextAddSeqsCmd(bioPtr, interp, objc, objv) != TCL_OK) goto error;
	break;
    case Biotext_CGET:
	if (objc != 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "option");
	    goto error;
	}
	resultObjPtr = Tk_GetOptionValue(interp, (char *) bioPtr, bioPtr->optionTable, objv[2], bioPtr->tkwin);
	if (resultObjPtr == NULL) {
	    result = TCL_ERROR;
	} else {
	    Tcl_SetObjResult(interp, resultObjPtr);
	}
	break;
    case Biotext_CLEAN:
	BiotextCleanCmd(bioPtr, interp, objc, objv);
	result = TCL_OK;

	break;
    case Biotext_CONFIGURE:
	resultObjPtr = NULL;
	if (objc <= 3) {
	    resultObjPtr = Tk_GetOptionInfo(interp, (char *) bioPtr, bioPtr->optionTable, (objc == 3) ? objv[2] : NULL, bioPtr->tkwin);
	    if (resultObjPtr == NULL) {
		Tcl_Release(bioPtr);
		return TCL_ERROR;
	    }

	    Tcl_SetObjResult(interp, resultObjPtr);
	} else {
	    /* set the configuration options */
	    result = BiotextConfigure(interp, bioPtr, objc-2, objv+2);
	}

	break;
	/* 
	   case Biotext_SELECTION:
	   if (objc < 3) {
	   Tcl_WrongNumArgs(interp, 3, objv, "option arg ?arg?");
	   goto error;
	    return TCL_ERROR;
	}
	   }
	   BiotextSelectionCmd(bioPtr, interp, objc, objv);
	   break;
	*/
    case Biotext_SEQUENCES:
	if (BiotextSequencesCmd(bioPtr, interp, objc, objv) != TCL_OK) goto error;
	break;
    case Biotext_INSERT:
	if (BiotextInsertCmd(bioPtr, interp, objc, objv) == TCL_ERROR) goto error;
	break;
    case Biotext_DELETE:
	if (BiotextDeleteCmd(bioPtr, interp, objc, objv) == TCL_ERROR) goto error;
	break;
    case Biotext_MAPPING:
	if (BiotextMappingCmd(bioPtr, interp, objc, objv) != TCL_OK) goto error;
	break;
    case Biotext_TAG:
	if (BiotextTagCmd(bioPtr, interp, objc, objv) != TCL_OK) goto error;
	break;
    case Biotext_MAP:
	if (BiotextMapCmd(bioPtr, interp, objc,objv) != TCL_OK) goto error;
	break;
    case Biotext_XVIEW:
	if (BiotextXViewCmd(bioPtr, interp, objc, objv) != TCL_OK) goto error;
	break;
    case Biotext_YVIEW:
	if (BiotextYViewCmd(bioPtr, interp, objc, objv)) goto error;
	break;
    case Biotext_GROUP:
	if (BiotextGroupCmd(bioPtr, interp, objc, objv) != TCL_OK) goto error;
	break;
    case Biotext_UNGROUP:
	if (BiotextUngroupCmd(bioPtr, interp, objc, objv) != TCL_OK) goto error;
	break;
    case Biotext_SEE:
	if (BiotextSeeCmd(bioPtr, interp, objc, objv) != TCL_OK) goto error;
	break;
    case Biotext_INDEX:
	if (BiotextIndexCmd(bioPtr, interp, objc, objv) != TCL_OK) goto error;
	break;
    case Biotext_PUSH:
	if (BiotextPushCmd(bioPtr, interp, objc, objv) != TCL_OK) goto error;
	break;
    case Biotext_CURSOR:
	if (BiotextCursorCmd(bioPtr, interp, objc, objv) != TCL_OK) goto error;
	break;
    case Biotext_CUT:
	if (BiotextCutCmd(bioPtr, interp, objc, objv) != TCL_OK) goto error;
	break;
    case Biotext_COPY:
	if (BiotextCopyCmd(bioPtr, interp, objc, objv) != TCL_OK) goto error;
	break;
    case Biotext_PASTE:
	if (BiotextPasteCmd(bioPtr, interp, objc, objv) != TCL_OK) goto error;
	break;
    case Biotext_FONT:
	if (BiotextFontCmd(bioPtr, interp, objc, objv) != TCL_OK) goto error;
	break;
    case Biotext_OUTPUT:
	if (BiotextOutputCmd(bioPtr, interp, objc, objv) != TCL_OK) goto error;
	break;
    case Biotext_DELCHARS:
	if (BiotextDelcharsCmd(bioPtr, interp, objc, objv) != TCL_OK) goto error;
	break;
    case Biotext_LOCK:
	if (BiotextLockCmd(bioPtr, interp, objc, objv) != TCL_OK) goto error;
	break;
    case Biotext_CHARS:
	if (BiotextCharsCmd(bioPtr, interp, objc, objv) != TCL_OK) goto error;
    }
    
    Tcl_Release(bioPtr);
    return result;

 error:
    Tcl_Release(bioPtr);
    return TCL_ERROR;
}


/*
*----------------------------------------------------------------------
 *
 * BiotextFocusProc --
 *
 *	This function is called whenever the biotext gets or loses the input
 *	focus. It's also called whenever the window is reconfigured while it
 *	has the focus.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The cursor gets turned on or off.
 *
 *----------------------------------------------------------------------
 */


static void
BiotextFocusProc(
    Biotext *bioPtr,		/* Biotext that got or lost focus. */
    int gotFocus)		/* 1 means window is getting focus, 0 means
				 * it's losing it. */
{
    Tcl_DeleteTimerHandler(bioPtr->cursorTimer);
    if (gotFocus) {
	bioPtr->flags |= GOT_FOCUS | CURSOR_ON;
	if (bioPtr->insertOffTime != 0) {
	    bioPtr->cursorTimer = Tcl_CreateTimerHandler(
								  bioPtr->insertOnTime, BiotextConfigCursor, bioPtr);
	}
    } else {
	/* no focus */
	bioPtr->flags &= ~(GOT_FOCUS | CURSOR_ON);
	bioPtr->cursorTimer = NULL;
    }

    BiotextEventuallyRedraw(bioPtr);

    return;
}


/*
 *----------------------------------------------------------------------
 *
 * BiotextConfigCursor --
 *
 *	Configures the timer depending on the 
 *      state of the Biotext widget.
 *	Equivalent to BiotextFocusProc.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The cursor will be switched off/on.
 *
 *----------------------------------------------------------------------
 */
static void
BiotextConfigCursor(void *clientData) 
{
    Biotext *bioPtr = (Biotext *)clientData;

    if ((bioPtr->state == STATE_DISABLED) || (bioPtr->tkwin == NULL) || (! (Tk_IsMapped(bioPtr->tkwin)))) {
	return;
    }

    /* make sure nothing existed */
    Tcl_DeleteTimerHandler(bioPtr->cursorTimer);

    /*
     * To have a cursor, we have to have focus and 
     * allow edits
     */
    if (bioPtr->flags & GOT_FOCUS) {
	/*
	 * Turn the cursor ON
	 */
	if (! (bioPtr->flags & CURSOR_ON)) {
	    bioPtr->flags |= CURSOR_ON;
	}
    
	/* set up the first timer */
	if (bioPtr->insertOffTime != 0) {
	    bioPtr->cursorTimer = Tcl_CreateTimerHandler(bioPtr->insertOnTime, BiotextCursorEvent, (ClientData) bioPtr);
	}
    } else {
	/* No focus
	 * Turn the cursor OFF
	 */
	if ((bioPtr->flags & CURSOR_ON)) {
	    bioPtr->flags &= ~CURSOR_ON;
	}
    
	/* and disable the timer */
	if (bioPtr->cursorTimer != NULL) {
	    Tcl_DeleteTimerHandler(bioPtr->cursorTimer);
	}
	bioPtr->cursorTimer = NULL;
    }
  
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * BiotextConfigure --
 *
 *	This procedure is called to process an 
 *      argv/argc list in conjunction
 *	with the Tk option database to configure (or 
 *      reconfigure) a Biotext
 *	widget.
 *
 * Results:
 *	The return value is a standard Tcl result. 
 *      If TCL_ERROR is returned, then the 
 *      interp's result contains an error message.
 *
 * Side effects:
 *	Configuration information, such as 
 *      colors, border width, etc. get set
 *	for bioPtr; old resources get freed, 
 *      if there were any.
 *
 *----------------------------------------------------------------------
 */
static int
BiotextConfigure(Tcl_Interp *interp, 
		 register Biotext *bioPtr,
		 int objc,
		 Tcl_Obj *const objv[])	
{
    int error;
    Tk_SavedOptions savedOptions;
    Tk_3DBorder bgBorder;
    Tcl_Obj *errorResult = NULL;
  
    for (error = 0; error <= 1; error++) {
	if (! error) {
	    /*
	     * First pass: set options to new values.
	     */
      
	    if (Tk_SetOptions(interp,
			      (char *) bioPtr,
			      bioPtr->optionTable,
			      objc, 
			      objv,
			      bioPtr->tkwin, 
			      &savedOptions, 
			      NULL) != TCL_OK) {
		continue;
	    }
	} else {
	    /*
	     * Second pass: 
	     * restore options to old values.
	     */
      
	    errorResult = Tcl_GetObjResult(interp);
	    Tcl_IncrRefCount(errorResult);
	    Tk_RestoreSavedOptions(&savedOptions);
	}
    
	/*
	 * Set the background for the window and create
	 * a graphics context for use during redisplay.
	 */
    
	bioPtr->XfgColor = Tk_3DBorderColor(bioPtr->fgBorder);
	bioPtr->XbgColor = Tk_3DBorderColor(bioPtr->bgBorder);
#ifdef USE_XFT
	XColor2XftColor(bioPtr->XfgColor, &(bioPtr->fgColor));
	XColor2XftColor(bioPtr->XbgColor, &(bioPtr->bgColor));
#endif
    
	bgBorder = bioPtr->bgBorder;
	Tk_SetBackgroundFromBorder(bioPtr->tkwin, bgBorder);
    
	/*
	 * Restart the cursor timing sequence in case the 
	 * on-time or off-time just changed. 
	 */
	if (bioPtr->flags & GOT_FOCUS) {
	    BiotextFocusProc(bioPtr, 1);
	}

	if (bioPtr->highlightWidth < 0) {
	    bioPtr->highlightWidth = 0;
	}
	bioPtr->inset = bioPtr->borderWidth + bioPtr->highlightWidth;
    
	break;
    }
#ifdef DEBUG_CONFIGURECMD
    fprintf(stderr,"\n== in BiotextConfigure =========\n");
    fprintf(stderr,"widget %s\n", bioPtr->widgetName);
    fprintf(stderr, "height= %d width= %d \n", bioPtr->heightChar, bioPtr->widthChar);
    fprintf(stderr,"------------------\n");
    fflush(stderr);
#endif

    if (! error) {
	//bioPtr->flags |= TAG_ALIGNMENT;
	Tk_FreeSavedOptions(&savedOptions);
    } else {
	Tcl_SetObjResult(interp, errorResult);
	Tcl_DecrRefCount(errorResult);

	return TCL_ERROR;
    }
  
    // on est dans BiotextConfigure
    //BiotextSetFont(bioPtr);
    //BiotextComputeGeometry(bioPtr, 1, 1, 1);
    // cf tkListbox.c
    BiotextWorldChanged(bioPtr);
  
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * BiotextObjEventProc --
 *
 *	This procedure is invoked by the Tk dispatcher for 
 *      various events on Biotexts.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	When the window gets deleted, internal structures get 
 *      cleaned up. When it gets exposed, it is redisplayed.
 *
 *--------------------------------------------------------------
 */

static void
BiotextObjEventProc(ClientData clientData,	/* Information about window. */
		    XEvent *eventPtr)		/* Information about event. */
{
    register Biotext *bioPtr = clientData;
    int width, height;
    int bdi;

    switch (eventPtr->type) {
    case DestroyNotify:
	if (! (bioPtr->flags & BIOTEXT_DELETED)) {
	    bioPtr->flags |= BIOTEXT_DELETED;

	    if (bioPtr->cursorTimer != NULL) {
		Tcl_DeleteTimerHandler(bioPtr->cursorTimer);
		bioPtr->cursorTimer = NULL;
	    }
	    if (bioPtr->insertBlinkHandler != NULL) {
		Tcl_DeleteTimerHandler(bioPtr->insertBlinkHandler);
		bioPtr->insertBlinkHandler = NULL;
	    }

	    Tcl_DeleteCommandFromToken(bioPtr->interp,bioPtr->widgetCmd);
	    if (bioPtr->setGrid) {
		Tk_UnsetGrid(bioPtr->tkwin);
	    }
	    if (bioPtr->flags & REDRAW_PENDING) {
		Tcl_CancelIdleCall(BiotextDisplay, clientData);
	    }
	    Tcl_EventuallyFree(clientData, (Tcl_FreeProc *) BiotextCmdDeletedProc);
	}
	break;
    case Expose:
	bioPtr->flags |= UPDATE_SCROLLBAR;
	BiotextEventuallyRedraw(bioPtr);
	break;
    case ConfigureNotify:
	/*
	 * Recalculate the number of lines/columns
	 * Then arrange for the window to be redisplayed.
	 */    
#ifdef DEBUG_CONFIGNOTIFY
	fprintf(stderr, "\n== ConfigureNotify =======\n");
	fprintf(stderr, "widget %s\n", bioPtr->widgetName);
	fprintf(stderr, "width in  %d height in  %d\n", bioPtr->widthChar, bioPtr->heightChar);
	fflush(stderr);
#endif
	Tcl_Preserve(bioPtr);
	bdi = bioPtr->borderWidth + bioPtr->highlightWidth;
	height = Tk_Height(bioPtr->tkwin);
	width  = Tk_Width(bioPtr->tkwin);
    
	bioPtr->heightChar = (height - 2 * bdi) / bioPtr->lineHeight;
	bioPtr->widthChar  = (width - 2 * bdi)  / bioPtr->charWidth;

	bioPtr->flags |= UPDATE_SCROLLBAR;
	ChangeBiotextXView(bioPtr, bioPtr->leftIndex);
	ChangeBiotextYView(bioPtr, bioPtr->topIndex);
#ifdef DEBUG_CONFIGNOTIFY
	fprintf(stderr, "width out %d height out %d\n", bioPtr->widthChar, bioPtr->heightChar);
	fprintf(stderr,"---------\n");
	fflush(stderr);
#endif
    
	BiotextEventuallyRedraw(bioPtr);
	Tcl_Release(bioPtr);
	break;
    case FocusIn:
    case FocusOut:
	if (eventPtr->xfocus.detail != NotifyInferior) {
	    BiotextFocusProc(bioPtr, (eventPtr->type == FocusIn));
	}
	break;
    }
  
    return;
}


/*
 *----------------------------------------------------------------------
 *
 * BiotextDeletedProc --
 *
 *	This procedure is invoked when a widget command is deleted. If the
 *	widget isn't already in the process of being destroyed, this command
 *	destroys it.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The widget is destroyed.
 *
 *----------------------------------------------------------------------
 */
 
static void
BiotextDeletedProc(clientData)
    ClientData clientData; /* Pointeur vers la structure Biotext */
{
    Biotext *bioPtr = (Biotext *) clientData;
    Tk_Window tkwin = bioPtr->tkwin;
    int i;


    /* first of all, clean the option table. */
    if (bioPtr->optionTable != NULL) {
        /* Pass NULL as third argument , tkwin not needed */
        Tk_FreeConfigOptions((char *) bioPtr, bioPtr->optionTable, NULL);
    }

    /* 1.Stop timers , cursor blinking */
    if (bioPtr->cursorTimer != NULL) {
        Tcl_DeleteTimerHandler(bioPtr->cursorTimer);
    }
    if (bioPtr->insertBlinkHandler != NULL) {
        Tcl_DeleteTimerHandler(bioPtr->insertBlinkHandler);
    }

    /* 2. Free graphics ressources : X11 and Xft */
    if (bioPtr->display != NULL) {
#ifdef USE_XFT
        if (bioPtr->xftFont != NULL) {
            XftFontClose(bioPtr->display, bioPtr->xftFont);
        }
        /* Free global XFT colors */
        XftColorFree(bioPtr->display, Tk_Visual(tkwin), Tk_Colormap(tkwin), &bioPtr->fgColor);
        XftColorFree(bioPtr->display, Tk_Visual(tkwin), Tk_Colormap(tkwin), &bioPtr->bgColor);
#endif
#ifdef WIN32
        if (bioPtr->winFont != NULL) {
            Tk_FreeFont(bioPtr->winFont);
            bioPtr->winFont = NULL;
        }
#endif
        if (bioPtr->gc != None) {
            Tk_FreeGC(bioPtr->display, bioPtr->gc);
        }
    }

    /* 3. Free strings and options */
    /* Bug fix: className, xscrollCmd and yscrollCmd are TK_OPTION_STRING
     * entries in BiotextOptionSpecs (-class, -xscrollcommand,
     * -yscrollcommand). Their memory is owned by Tk and was ALREADY
     * released by Tk_FreeConfigOptions() above (step 1). Tk does not
     * NULL the struct fields afterward, so they are dangling (non-NULL)
     * pointers at this point; ckfree()'ing them again here was a
     * double free on every single widget destruction. widgetName,
     * FontXft and FontName are NOT part of the option table (they are
     * managed entirely by this file's own code), so they still need
     * to be freed manually, unlike className/xscrollCmd/yscrollCmd. */
    if (bioPtr->widgetName != NULL) ckfree(bioPtr->widgetName);
    if (bioPtr->FontXft != NULL) ckfree(bioPtr->FontXft);
    if (bioPtr->FontName != NULL) ckfree(bioPtr->FontName);
    
    /* 4. Free data matrices (SÃ©quences, Map, Tags) */
    if (bioPtr->SeqMat != NULL) {
        for (i = 0; i < bioPtr->nbSeqs; i++) {
            if (bioPtr->SeqMat[i] != NULL) ckfree(bioPtr->SeqMat[i]);
        }
        ckfree((char *) bioPtr->SeqMat);
    }
    if (bioPtr->MapSeq != NULL) {
        for (i = 0; i < bioPtr->nbSeqs; i++) {
            if (bioPtr->MapSeq[i] != NULL) ckfree((char *) bioPtr->MapSeq[i]);
        }
        ckfree((char *) bioPtr->MapSeq);
    }
    if (bioPtr->TagSeq != NULL) {
        for (i = 0; i < bioPtr->nbSeqs; i++) {
            if (bioPtr->TagSeq[i] != NULL) ckfree((char *) bioPtr->TagSeq[i]);
        }
        ckfree((char *) bioPtr->TagSeq);
    }

    /* 5. Cleaning cache : cut copy and paste */
    if (bioPtr->SeqMatCache != NULL) {
        for (i = 0; i < bioPtr->nbSeqsCache; i++) {
            if (bioPtr->SeqMatCache[i] != NULL) ckfree(bioPtr->SeqMatCache[i]);
        }
        ckfree((char *) bioPtr->SeqMatCache);
    }
    if (bioPtr->MapSeqCache != NULL) {
        for (i = 0; i < bioPtr->nbSeqsCache; i++) {
            if (bioPtr->MapSeqCache[i] != NULL) ckfree((char *) bioPtr->MapSeqCache[i]);
        }
        ckfree((char *) bioPtr->MapSeqCache);
    }

    /* 6. Free Tcl objects, reference counters */
    if (bioPtr->listTags != NULL) Tcl_DecrRefCount(bioPtr->listTags);
    if (bioPtr->listTagsCache != NULL) Tcl_DecrRefCount(bioPtr->listTagsCache);
    if (bioPtr->GroupList != NULL) Tcl_DecrRefCount(bioPtr->GroupList);

    /* 7 Free simple tables */
    if (bioPtr->Mapping != NULL) ckfree((char *) bioPtr->Mapping);
    if (bioPtr->Tags != NULL) ckfree((char *) bioPtr->Tags);
    if (bioPtr->SeqGrp != NULL) ckfree((char *) bioPtr->SeqGrp);
    if (bioPtr->SeqGrpCache != NULL) ckfree((char *) bioPtr->SeqGrpCache);

    /* Finally free the whole widget structure */
    Tcl_EventuallyFree((ClientData) bioPtr, TCL_DYNAMIC);
    //ckfree((char *) bioPtr);
}


/*
 *----------------------------------------------------------------------
 *
 * BiotextEventuallyRedraw --
 *
 *	Ensure that a biotext is eventually redrawn 
 *      on the display.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Information gets redisplayed. Right now we 
 *      don't do selective redisplays: the whole 
 *      window will be redrawn.
 *      This doesn't seem to hurt performance 
 *      noticeably, but if it does then this could be
 *	changed.
 *
 *----------------------------------------------------------------------
 */
static void
BiotextEventuallyRedraw(register Biotext *bioPtr) 
{
    if ((bioPtr->flags & REDRAW_PENDING) || (bioPtr->flags & BIOTEXT_DELETED) || (bioPtr->tkwin == NULL) || ! Tk_IsMapped(bioPtr->tkwin)) {
	return;
    }
  
    /*
     * Right now we don't do selective redisplays: 
     * the whole window will be redrawn. This doesn't 
     * seem to hurt performance noticeably, but if it
     * does then this could be changed.
     */
  
    bioPtr->flags |= REDRAW_PENDING;
    Tcl_DoWhenIdle(BiotextDisplay, bioPtr);

    return;
}

/*
 *--------------------------------------------------------------
 *
 * BiotextDisplay --
 *
 *	This procedure redraws the contents of a Biotext window.
 *      It is invoked as a do-when-idle handler, so it only runs
 *      when there's nothing else for the application to do.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Information appears on the screen.
 *
 *--------------------------------------------------------------
 */
static void
BiotextDisplay(ClientData clientData) 
{
    register Biotext *bioPtr = clientData;
    register Tk_Window tkwin = bioPtr->tkwin;
    Display *display = bioPtr->display;
    Pixmap pm = None;
    Drawable d;
  
    /*
     * reset the pending flag
     */
    bioPtr->flags &= ~REDRAW_PENDING;
    if ((bioPtr->flags & BIOTEXT_DELETED) || ! Tk_IsMapped(tkwin)) {
	return;
    }
  
    /*
     * Update scrollbars if that's needed.
     */
  
    if (bioPtr->flags & UPDATE_SCROLLBAR) {
	bioPtr->flags &= ~UPDATE_SCROLLBAR;

	Tcl_Preserve(bioPtr);
	BiotextUpdateYScrollbar(bioPtr);
	if ((bioPtr->flags & BIOTEXT_DELETED) || ! Tk_IsMapped(tkwin)) {
	    Tcl_Release(bioPtr);
	    return;
	}
	BiotextUpdateXScrollbar(bioPtr);
	if ((bioPtr->flags & BIOTEXT_DELETED) || ! Tk_IsMapped(tkwin)) {
	    Tcl_Release(bioPtr);
	    return;
	}
	Tcl_Release(bioPtr);
    }
  
    /*
     * Create a pixmap for doublebuffering if necessary.
     */
#ifndef TK_NO_DOUBLE_BUFFERING
    pm = Tk_GetPixmap(display, Tk_WindowId(tkwin), Tk_Width(tkwin), Tk_Height(tkwin), DefaultDepthOfScreen(Tk_Screen(tkwin)));
    d = pm;
#else
    d = Tk_WindowId(tkwin);
#endif
  
    /*
     * Redraw the widget's background and border.
     */
    Tk_Fill3DRectangle(tkwin, d, bioPtr->bgBorder, 0, 0, Tk_Width(tkwin), Tk_Height(tkwin), 0, TK_RELIEF_FLAT);
  
    /*
     * Display the Biotext.
     */
  
    GC newgc;
    XGCValues gcValues;
    unsigned long mask;
  
    gcValues.graphics_exposures = False;
    gcValues.font = Tk_FontId(bioPtr->tkfont);
    gcValues.foreground = Tk_3DBorderColor(bioPtr->fgBorder)->pixel;
    //gcValues.background = Tk_3DBorderColor(bioPtr->bgBorder)->pixel;
    mask = GCForeground | GCFont | GCGraphicsExposures;
    newgc = Tk_GetGC(tkwin, mask,&gcValues);
    if (bioPtr->gc != NULL) {
	Tk_FreeGC(bioPtr->display,bioPtr->gc);
    }
    bioPtr->gc = newgc;
  
    /*
     * If Empty widget, then return.
     */
    if (bioPtr->nbSeqs == 0) {
	/* Bug fix: when TK_NO_DOUBLE_BUFFERING is defined, "pm" stays
	 * None (no pixmap was ever created above, "d" is the real
	 * window instead) -- XCopyArea from a None source and
	 * Tk_FreePixmap(None) are both invalid X calls in that build
	 * configuration. Only do the copy/free when a pixmap actually
	 * exists. Low practical impact since TK_NO_DOUBLE_BUFFERING is
	 * not defined in normal builds, but worth being correct. */
#ifndef TK_NO_DOUBLE_BUFFERING
	XCopyArea(display, pm, Tk_WindowId(tkwin), bioPtr->gc, 0, 0, (unsigned) Tk_Width(tkwin), (unsigned) Tk_Height(tkwin), 0, 0);
	Tk_FreePixmap(display, pm);
	/*
	 * Force the copy out to the X server right away instead of
	 * leaving it sitting in Xlib's client-side output buffer.
	 * Tk normally flushes automatically just before the event loop
	 * blocks waiting for the next event, so this is a no-op in
	 * practice on a local display. But without backing store (e.g.
	 * over "ssh -Y" X11 forwarding), the window has no valid pixels
	 * at all until this XCopyArea actually reaches the server, so any
	 * delay before the next automatic flush shows up as a black
	 * rectangle -- which then "heals" as soon as some other X traffic
	 * (a mouse-motion event, for instance) forces a flush. An explicit
	 * XFlush here closes that gap.
	 */
	XFlush(display);
#endif
    
	return;
    }
  
    /*
     * Draw the character matrix according to the 
     * current mapping.
     */
  
    BiotextDrawAlignment(bioPtr, d);
  
    /*
     * Draw the blinking block cursor if any
     */
    if ((bioPtr->state == STATE_NORMAL) && (bioPtr->flags & GOT_FOCUS) && (bioPtr->flags & CURSOR_ON)) {
	int xP, yP, wc, hc, col, row;
    
	col = bioPtr->currentC;
	row = bioPtr->currentR;
	if (Biotext_IsVisible(bioPtr, col, row, 1)) {
	    wc  = bioPtr->charWidth;
	    hc  = bioPtr->charHeight;
	    Biotext_Coords2Pixels(bioPtr, col, row, &xP, &yP, 1);
      
	    Tk_Fill3DRectangle(tkwin, d, Tk_Get3DBorder(bioPtr->interp, tkwin, Tk_NameOfColor(bioPtr->cursorColor)), xP, yP, wc, hc, 1, TK_RELIEF_RAISED);
	}
    }
  
    Tk_Draw3DRectangle(tkwin, pm, bioPtr->bgBorder, bioPtr->highlightWidth, bioPtr->highlightWidth, Tk_Width(tkwin) - 2*bioPtr->highlightWidth, Tk_Height(tkwin) - 2*bioPtr->highlightWidth, bioPtr->borderWidth, bioPtr->relief);
  
    /*
     * If double-buffered, copy to the screen and 
     * release the pixmap.
     */
#ifndef TK_NO_DOUBLE_BUFFERING
    //XGCValues gcValues;
    //gcValues.foreground = Tk_3DBorderColor(bioPtr->fgBorder)->pixel;
    //GC newgc = bioPtr->gc;
    //XChangeGC(display, newgc, GCForeground, &gcValues);
    XCopyArea(display, pm, Tk_WindowId(tkwin), bioPtr->gc, 0, 0, (unsigned) Tk_Width(tkwin), (unsigned) Tk_Height(tkwin), 0, 0);
    Tk_FreePixmap(display, pm);
    /*
     * See the comment on the XFlush() call above (empty-widget case):
     * force this copy out to the X server immediately rather than
     * leaving it queued client-side, so the window never sits without
     * valid pixels -- which is what a missing backing store turns into
     * a visible black rectangle -- for longer than necessary.
     */
    XFlush(display);
#endif

    return;
}

/*
 *----------------------------------------------------------------------
 *
 * BiotextCmdDeletedProc --
 *
 *	This procedure is invoked by 
 *      Tcl_EventuallyFree or Tcl_Release to
 *	clean up the internal structure of a Biotext 
 *      at a safe time when no-one is using it anymore
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Everything associated with the Biotext is 
 *      freed up.
 *
 *----------------------------------------------------------------------
 */
static void
BiotextCmdDeletedProc(void *memPtr) /* Info about Biotext widget. */
{
    register Biotext *bioPtr = memPtr;

    /*
     * This procedure could be invoked either because the window was destroyed
     * and the command was then deleted (in which case tkwin is NULL) or
     * because the command was deleted, and then this procedure destroys the
     * widget.
     */
  
    if (!(bioPtr->flags & BIOTEXT_DELETED)) {
	Tk_DestroyWindow(bioPtr->tkwin);
    }

}


/*
 *----------------------------------------------------------------------
 *
 * BiotextVisibleXRange --
 *
 *	Return information about the range of the 
 *      Biotext in X-coord that is currently
 *	visible.
 *
 * Results:
 *	*firstPtr and *lastPtr are modified to hold 
 *      fractions between 0 and 1
 *	identifying the range of characters visible 
 *      in the Biotext.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static void
BiotextVisibleXRange(register Biotext *bioPtr,
		     double *firstPtr,  
		     double *lastPtr) 
{
    if (bioPtr->lgSeqs <= bioPtr->widthChar) {
	bioPtr->leftIndex = 0;
	*firstPtr = 0.0;
	*lastPtr  = 1.0;
    } else {
	*firstPtr = (double)bioPtr->leftIndex / (double)bioPtr->lgSeqs;
	*lastPtr = (double)(bioPtr->leftIndex + bioPtr->widthChar) / (double)bioPtr->lgSeqs;
	
	if (*lastPtr > 1.0) {
	    *lastPtr = 1.0;
	}
    }

    return;
}
  
/*
 *----------------------------------------------------------------------
 *
 * BiotextVisibleYRange --
 *
 *	Return information about the range of the 
 *      Biotext in Y-coord that is currently
 *	visible.
 *
 * Results:
 *	*firstPtr and *lastPtr are modified to hold 
 *      fractions between 0 and 1 inclusive
 *	identifying the range of characters visible 
 *      in the Biotext.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static void
BiotextVisibleYRange(register Biotext *bioPtr,
		     double *firstPtr,
		     double *lastPtr) 
{
    if (bioPtr->nbSeqs <= bioPtr->heightChar) {
	bioPtr->topIndex = 0;
	*firstPtr = 0.0;
	*lastPtr  = 1.0;
    } else {
	*firstPtr = (double)bioPtr->topIndex / (double)bioPtr->nbSeqs;
	*lastPtr = (double)(bioPtr->topIndex + bioPtr->heightChar) / (double)bioPtr->nbSeqs;
    
	if (*lastPtr > 1.0) {
	    *lastPtr = 1.0;
	}
    }
  
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * BiotextUpdateXScrollbar --
 *
 *	This function is invoked whenever 
 *      information has changed in an Biotext
 *	in a way that would invalidate a X -xscrollbar
 *      display. If there is an
 *	associated X -scrollbar, then this function 
 *      updates it by invoking a Tcl
 *	command.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A Tcl command is invoked, and an additional 
 *      command may be
 *	invoked to process errors in the command.
 *
 *----------------------------------------------------------------------
 */

static void
BiotextUpdateXScrollbar(register Biotext *bioPtr) 
{
    Tcl_Interp *interp;
    int code;
    char firstStr[TCL_DOUBLE_SPACE], lastStr[TCL_DOUBLE_SPACE];
    double first, last;
    Tcl_DString buf;

    if (bioPtr->xscrollCmd == NULL) {
	return;
    }

    interp = bioPtr->interp;

    BiotextVisibleXRange(bioPtr, &first, &last);

    Tcl_Preserve(interp);
    Tcl_PrintDouble(NULL, first, firstStr);
    Tcl_PrintDouble(NULL, last, lastStr);

    Tcl_DStringInit(&buf);
    Tcl_DStringAppend(&buf, bioPtr->xscrollCmd, -1);
    Tcl_DStringAppend(&buf, " ", -1);
    Tcl_DStringAppend(&buf, firstStr, -1);
    Tcl_DStringAppend(&buf, " ", -1);
    Tcl_DStringAppend(&buf, lastStr, -1);
    code = Tcl_EvalEx(interp, Tcl_DStringValue(&buf), -1, 0);
    Tcl_DStringFree(&buf);

    if (code != TCL_OK) {
	Tcl_AppendObjToErrorInfo(interp, Tcl_ObjPrintf("\n    (horizontal scrolling command executed by %s)",Tk_PathName(bioPtr->tkwin)));
	Tcl_BackgroundException(interp, code);
    }
    Tcl_ResetResult(interp);
    Tcl_Release(interp);

    return;
}


/*
 *----------------------------------------------------------------------
 *
 * BiotextUpdateYScrollbar --
 *
 *	This function is invoked whenever 
 *      information has changed in an Biotext
 *	in a way that would invalidate an
 *      Y -yxscrollbar display. If there is an
 *	associated X -scrollbar, then this function 
 *      updates it by invoking a Tcl
 *	command.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A Tcl command is invoked, and an additional command may be
 *	invoked to process errors in the command.
 *
 *----------------------------------------------------------------------
 */
static void
BiotextUpdateYScrollbar(register Biotext *bioPtr) 
{
    Tcl_Interp *interp;
    int code;
    char firstStr[TCL_DOUBLE_SPACE], lastStr[TCL_DOUBLE_SPACE];
    double first, last;
    Tcl_DString buf;
  
    if (bioPtr->yscrollCmd == NULL) {
	return;
    }

    interp = bioPtr->interp;

    BiotextVisibleYRange(bioPtr, &first, &last);

    Tcl_Preserve(interp);
    Tcl_PrintDouble(NULL, first, firstStr);
    Tcl_PrintDouble(NULL, last, lastStr);
  
    Tcl_DStringInit(&buf);
    Tcl_DStringAppend(&buf, bioPtr->yscrollCmd, -1);
    Tcl_DStringAppend(&buf, " ", -1);
    Tcl_DStringAppend(&buf, firstStr, -1);
    Tcl_DStringAppend(&buf, " ", -1);
    Tcl_DStringAppend(&buf, lastStr, -1);

    code = Tcl_EvalEx(interp, Tcl_DStringValue(&buf), -1, 0);
    Tcl_DStringFree(&buf);
  
    if (code != TCL_OK) {
	Tcl_AppendObjToErrorInfo(interp, Tcl_ObjPrintf("\n    (vertical scrolling command executed by %s)",Tk_PathName(bioPtr->tkwin)));
	Tcl_BackgroundException(interp, code);
    }
    Tcl_ResetResult(interp);

    Tcl_Release(interp);
  
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * BiotextCursorEvent --
 *	Toggle the cursor status. 
 *      Equivalent to EntryBlinkProc.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The cursor will be switched off/on.
 *
 *----------------------------------------------------------------------
 */
static void
BiotextCursorEvent(ClientData clientData)
{
    register Biotext *bioPtr = (Biotext *) clientData;

    if ((bioPtr->state == STATE_DISABLED) ||
	!(bioPtr->flags & GOT_FOCUS) || (bioPtr->insertOffTime == 0)) {
	return;
    }
    
    if (bioPtr->flags & CURSOR_ON) {
	bioPtr->flags &= ~CURSOR_ON;
	bioPtr->cursorTimer = Tcl_CreateTimerHandler(bioPtr->insertOffTime, BiotextCursorEvent, (ClientData) bioPtr);
    } else {
	bioPtr->flags |= CURSOR_ON;
	bioPtr->cursorTimer = Tcl_CreateTimerHandler(bioPtr->insertOnTime, BiotextCursorEvent, (ClientData) bioPtr);
    }

    /* Redraw */
    BiotextEventuallyRedraw(bioPtr);

    return;
}


/*
 * Package initialisation
 */
int DLLEXPORT Biotext_Init(Tcl_Interp *interp) 
{
    /* "8.6-" (open-ended range) accepts both Tcl/Tk 8.6.x and any 9.x,
     * instead of "8.6" alone which only matches the 8.x line. */
    if (Tcl_InitStubs(interp, "8.6-" , 0) == NULL) {
	return TCL_ERROR;
    }
    if (Tk_InitStubs(interp, "8.6-" , 0) == NULL) {
	return TCL_ERROR;
    }
  
    Tcl_CreateObjCommand(interp, "biotext", BiotextObjCmd, (ClientData) NULL, (Tcl_CmdDeleteProc *)NULL);
  
    Tcl_PkgProvide(interp,"biotext","1.0");
  
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * BiotextWorldChanged --
 *
 *	This function is called when the world has changed in 
 *      some way and tahe widget needs to recompute all its 
 *      graphics contexts and determine its new geometry.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Biotext will be relayed out and redisplayed.
 *
 *---------------------------------------------------------------------------
 */
static void
BiotextWorldChanged(ClientData instanceData) 
{
    Biotext *bioPtr = instanceData;
    XGCValues gcValues;
    GC gc = None;
  
    Tk_SetBackgroundFromBorder(bioPtr->tkwin, bioPtr->bgBorder);
  
    if (bioPtr->doubleBuffer) {
	/*
	 * Set up the graphical context:
	 * - defaultgc is used when no mapping 
	 */
	gcValues.graphics_exposures = False;
	gcValues.font = Tk_FontId(bioPtr->tkfont);
	gcValues.foreground = Tk_3DBorderColor(bioPtr->fgBorder)->pixel;
	gc = Tk_GetGC(bioPtr->tkwin,GCGraphicsExposures|GCFont|GCForeground, &gcValues);
	if (bioPtr->gc != NULL) {
	    Tk_FreeGC(bioPtr->display, bioPtr->gc);
	}
	bioPtr->gc = gc;
    }
  
    /*
     * Recompute the window's geometry and arrange for it to be redisplayed.
     */
#ifdef DEBUG_WORLDCHANGED
    fprintf(stderr, "\n== in BiotextWorldChanged ==========\n");
    fprintf(stderr, "widget %s\n", bioPtr->widgetName);
    fflush(stderr);
#endif
  
    BiotextSetFont(bioPtr);
    // in biotextworldchanged
    BiotextComputeGeometry(bioPtr, 1, 1,1);

    bioPtr->flags |= UPDATE_SCROLLBAR;
    BiotextEventuallyRedraw(bioPtr);
  
    return;
}
  

/* 
   valgrind -v --leak-check=yes --num-callers=10 --leak-resolution=high --show-reachable=no /home/moumou/tcl8.6.4/luinst/bin/tclsh tbiotext.tcl >& log
*/

/* ================================= 
 * : Widget commands
 * ================================ */

/*
 *--------------------------------------------------------------
 *
 * BiotextSelectionCmd --
 *
 *	This procedure is invoked to process the 
 *      "selection" method. It
 *	dispatches the sub-methods of "selction".
 *      Valid options are : from, to, clear
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None
 *
 *--------------------------------------------------------------
 */
static int 
BiotextSelectionCmd(Biotext *bioPtr,
		    Tcl_Interp *interp,
		    int objc,
		    Tcl_Obj * const objv[]) 
{
    int cmdIndex, r, c;
  
    static CONST84 char *selCmdNames[] = {
	"clear", "from", "to", (char *)NULL
    };
    enum selCommand {
	CMD_SEL_CLEAR, CMD_SEL_FROM, CMD_SEL_TO,
    };

    if (Tcl_GetIndexFromObj(interp, objv[2], selCmdNames, "selection option", 0, &cmdIndex) != TCL_OK) {
	return TCL_ERROR;
    }
    switch ((enum selCommand) cmdIndex) {
    case CMD_SEL_CLEAR:
	bioPtr->selFirstX = -1;
	bioPtr->selFirstY = -1;
	bioPtr->selLastX  = -1;
	bioPtr->selLastY  = -1;
	break;
    case CMD_SEL_FROM:
	if (objc != 4) {
	    return TCL_ERROR;
	}
	BiotextGetIndexObj(bioPtr, objv[3], &r, &c);

	bioPtr->selFirstX = r;
	bioPtr->selFirstY = c;
	break;
    case CMD_SEL_TO:
	if (objc != 4) {
	    return TCL_ERROR;
	}
	BiotextGetIndexObj(bioPtr, objv[3], &r, &c);

	bioPtr->selLastX  = r;
	bioPtr->selLastY  = c;
    }
  
    BiotextEventuallyRedraw(bioPtr);
  
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * BiotextAddSeqsCmd --
 *
 *	This procedure is invoked to process the 
 *      "addseqs" method. It
 *	adds a list of sequences to the widget,
 *      and display them. If possible, it maps
 *      each residue.
 *      Be aware that NO checking on the sequences is
 *      done at this level !
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None
 *
 *--------------------------------------------------------------
 */
static int 
BiotextAddSeqsCmd(register Biotext *bioPtr,
		  Tcl_Interp *interp,
		  int objc,
		  Tcl_Obj * const objv[]) 
{
    int i, k, nseq, slen, index, diff;
    char *iStr, dummy;
    Tcl_Obj **OSeqMat;
    Tcl_Size nseqSz, slenSz; /* Tcl 9: Tcl_ListObjGetElements()/Tcl_GetStringFromObj()
			       * write through a Tcl_Size* OUT param, not int*. */
  
    /* Sequences should already be present */
    if (bioPtr->nbSeqs <= 0) {
	return TCL_OK;
    }

    /* parse arguments */
    if (objc < 4) {
	Tcl_WrongNumArgs(interp, 1, objv, "addseqs seq-list index");
	return TCL_ERROR;
    }

    /* Get the insertion point */
    iStr = Tcl_GetString(objv[3]);
    /* Bug fix: same single-byte stack overflow as BiotextInsertCmd
     * ("%d%s" writing an unbounded string into a 1-byte "char dummy").
     * Use "%c" like BiotextGetIndex() does, which only ever writes
     * exactly one byte. */
    if (strcmp(iStr, "end") && (sscanf(iStr, "%d%c", &index, &dummy) != 1)) {
	Tcl_WrongNumArgs(interp, 1, objv, "addseqs <seq-list> index");
	return TCL_ERROR;
    }
    if (strcmp(iStr, "end") == 0) {
	/* Bug fix: this used to be "index = bioPtr->nbSeqs;". A few
	 * lines below, BiotextInsertRows(bioPtr, index, nseq) is called
	 * with a positive count, whose "insert AFTER index" semantics
	 * computes "start = index + 1". With index == nbSeqs (one past
	 * the last valid row, e.g. 4 when there are rows 0..3), that
	 * start becomes nbSeqs+1 (5) -- but BiotextInsertRows() only
	 * ever allocates new row buffers for indices
	 * bioPtr->nbSeqs..nbs-1 (i.e. just index 4 here), so writing to
	 * row 5 dereferenced an unallocated/garbage SeqMat[] slot.
	 * Confirmed by a real SIGSEGV in BiotextInsertRows ("SeqMat[i][j]
	 * = iMap;" with i == nbSeqs+1) when reproduced live. "end" must
	 * resolve to the last valid row (nbSeqs-1) so that "insert after"
	 * correctly targets the position right after it (nbSeqs). */
	index = bioPtr->nbSeqs - 1;
    }

    /* check inserted sequences */
    Tcl_ListObjGetElements(interp,objv[2],&nseqSz,&OSeqMat);
    nseq = (int)nseqSz;
    if (nseq == 0) {
	return TCL_OK;
    }

    /* Get inserted sequence length */
    Tcl_GetStringFromObj(OSeqMat[0],&slenSz);
    slen = (int)slenSz;

#ifdef DEBUG_ADDSEQS
    fprintf(stderr, "\n== BiotextAddSeqs =======\n");
    fprintf(stderr, "widget %s\n", bioPtr->widgetName);
    fprintf(stderr, "insertion index %d\n", index);
    fprintf(stderr, "new seq length %d old seqs %d\n", slen, bioPtr->lgSeqs);
    fflush(stderr);
#endif
  
    diff = bioPtr->lgSeqs - slen;
    if (diff < 0) {
	/* Inserted sequences are longer than the one
	 * in the alignment. Insert cols at the end.
	 */
	diff = abs(diff);
	if (BiotextInsertCols(bioPtr, bioPtr->lgSeqs-1, diff, -1) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
  
    /*
     * Insert empty rows after index
     * record previous number of seqs 
     */
    BiotextInsertRows(bioPtr, index, nseq);

    /* fill alignement with new sequence */
    for(k=0, i=index+1; i<index+1+nseq; i++, k++) {
	strcpy(bioPtr->SeqMat[i], Tcl_GetString(OSeqMat[k]));
	if (bioPtr->nbMapping > 0) {
	    /* map the new sequences */
	    /* Bug fix: bioPtr->MapSeq[i] was already allocated (and
	     * filled with the default gap mapping) by BiotextInsertRows()
	     * just above; overwriting it here without freeing the old
	     * buffer first leaked one row-sized int array per inserted
	     * sequence whenever mapping is active. (MapOneSequence()
	     * itself now resolves the fallback index for unmapped
	     * characters internally -- see its bug-fix note -- so no
	     * extra argument is needed here.) */
	    ckfree((char *)bioPtr->MapSeq[i]);
	    bioPtr->MapSeq[i] = MapOneSequence(bioPtr, bioPtr->SeqMat[i]);
	}
    }

    bioPtr->flags |= UPDATE_SCROLLBAR;
    BiotextEventuallyRedraw(bioPtr);

    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * BiotextSequencesCmd --
 *
 *	This procedure is invoked to process the 
 *      "sequences" method.
 *	It uploads a list of sequences into the widget,
 *      and displays them. If possible, it assign a tag
 *      to each residue.
 *      Be aware that no checking on the sequences is
 *      done at this level !
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None
 *
 *--------------------------------------------------------------
 */
static int 
BiotextSequencesCmd(Biotext *bioPtr,
		    Tcl_Interp *interp,
		    int objc,
		    Tcl_Obj * const objv[]) 
{
    int i, nseq, slen;
    Tcl_Obj **OSeqMat;
    Tcl_Size nseqSz, slenSz;

    if (objc !=3) {
	Tcl_WrongNumArgs(interp, 2, objv, "<sequences-list>");
	return TCL_ERROR;
    }

    Tcl_ListObjGetElements(interp, objv[2], &nseqSz, &OSeqMat);
    nseq = (int)nseqSz;
    if (nseq == 0) {
	return TCL_OK;
    }
  
    /*
     * cleaning 
     */
    //BiotextInitBuffer(bioPtr,0);
    if (bioPtr->SeqMat != NULL) {
 	for (i=0; i < bioPtr->nbSeqs; i++) {
	    ckfree((char *)bioPtr->SeqMat[i]);
	    if (bioPtr->nbMapping > 0) {
		ckfree((char *)bioPtr->MapSeq[i]);
	    }
	}
	ckfree((char *)bioPtr->SeqMat);
	if (bioPtr->nbMapping > 0) {
	    ckfree((char *)bioPtr->MapSeq);
	}
	ckfree((char *)bioPtr->SeqGrp);
	if (bioPtr->Tags != NULL) {
	    for (i=0; i< bioPtr->nbTags; i++) {
		/* Bug fix: same leak pattern already fixed in
		 * BiotextCleanCmd/BiotextTagDeleteSubCmd/
		 * BiotextRebuildTagTable -- coords was simply overwritten
		 * with NULL (leaking the Tcl_Obj it pointed to), and
		 * Cfg/Cbg/bgBorder/gc were never released at all. */
		if (bioPtr->Tags[i].coords != NULL) {
		    Tcl_DecrRefCount(bioPtr->Tags[i].coords);
		}
		bioPtr->Tags[i].coords = NULL;
		if (bioPtr->Tags[i].tagName != NULL) {
		    ckfree((char *) bioPtr->Tags[i].tagName);
		}
		if (bioPtr->Tags[i].Cfg != NULL) {
		    Tk_FreeColor(bioPtr->Tags[i].Cfg);
		}
		if (bioPtr->Tags[i].Cbg != NULL) {
		    Tk_FreeColor(bioPtr->Tags[i].Cbg);
		}
		if (bioPtr->Tags[i].bgBorder != NULL) {
		    Tk_Free3DBorder(bioPtr->Tags[i].bgBorder);
		}
		if (bioPtr->Tags[i].gc != NULL) {
		    Tk_FreeGC(bioPtr->display, bioPtr->Tags[i].gc);
		}
		//ckfree((char *) bioPtr->Tags[i]);
	    }
	    ckfree((char *) bioPtr->Tags);
	    bioPtr->Tags = NULL;
	}
	bioPtr->nbTagsAlloc = 0;
	bioPtr->nbTags = 0;
	bioPtr->listTags = Tcl_NewListObj(0, NULL);
	bioPtr->SeqMat = NULL;
	bioPtr->MapSeq = NULL;
	/* do NOT put nbMapping to zero ! 
	 * Should only be done by 'map cleear'
	 */
	//bioPtr->nbMapping = 0;
    }

    /*
     * Arrays allocation 
     */
    /* sequence length , WITHOUT \0 */
    Tcl_GetStringFromObj(OSeqMat[0],&slenSz);
    slen = (int)slenSz;
    /* Bug fix: "slen <= 1" also rejected perfectly valid length-1
     * sequences, while the error message only mentions length 0.
     * Match the check to the stated intent. */
    if (slen <= 0) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf("Sequences of length 0 !"));
	return TCL_ERROR;
    }
    /* Bug fix: only OSeqMat[0]'s length was ever checked. Every row is
     * then allocated to exactly (slen+1) bytes and filled with
     * strcpy(bioPtr->SeqMat[i], Tcl_GetString(OSeqMat[i])) below,
     * without ever verifying that every other sequence has that same
     * length -- a multiple-sequence alignment widget requires all
     * rows to have equal length anyway, and a longer sequence at any
     * index other than 0 (a very plausible mistake, e.g. pasting an
     * unaligned FASTA file) overflowed that (slen+1)-sized heap
     * buffer via strcpy. Validate all lengths up front instead. */
    for (i=1; i<nseq; i++) {
	int leni;
	Tcl_Size leniSz;
	Tcl_GetStringFromObj(OSeqMat[i], &leniSz);
	leni = (int)leniSz;
	if (leni != slen) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"all sequences must have the same length (sequence #%d has length %d, expected %d)",
		i, leni, slen));
	    return TCL_ERROR;
	}
    }

    /* Allocate and fill sequences array */
#ifdef DEBUG_SEQUENCESCMD
    fprintf(stderr, "\n== in BiotextSequenceseCmd =========\n");
    fprintf(stderr, "widget %s\n", bioPtr->widgetName);
    fprintf(stderr, "nseq %d slen %d\n", nseq, slen);
    fprintf(stderr, "------------------\n");
    fflush(stderr);
#endif
    bioPtr->SeqMat = (char **)ckalloc(nseq*sizeof(char *));
    for (i=0; i<nseq; i++) {
	bioPtr->SeqMat[i] = (char *)ckalloc((slen+1)*sizeof(char));
	strcpy(bioPtr->SeqMat[i],Tcl_GetString(OSeqMat[i]));
    }
    //DumpNullChar(bioPtr);
    bioPtr->nbSeqs  = nseq;
    bioPtr->lgSeqs  = slen;
  
    /* sequence groupe number */
    bioPtr->SeqGrp = (int *)ckalloc(nseq*sizeof(int));
    for (i=0; i<nseq; i++) {
	bioPtr->SeqGrp[i] = 0;
    }

    if (bioPtr->nbMapping > 0) {
	MapSequences(bioPtr);
    }  

    /* As the widget is filled, initialilze the cursor position to the top left corner */
    bioPtr->currentR = 0;
    bioPtr->currentC = 0;

    /* Redraw the widget */
    bioPtr->flags |= UPDATE_SCROLLBAR;
    BiotextEventuallyRedraw(bioPtr);
  
    return TCL_OK;
}


/*
 *--------------------------------------------------------------
 *
 * BiotextMappingCmd --
 *
 *	This procedure is invoked to process the 
 *      "mapping" method. It
 *	defines a new mapping as a list of 
 *      {key foreground background} items
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None
 *
 *--------------------------------------------------------------
 */
static int 
BiotextMappingCmd(register Biotext *bioPtr,
		  Tcl_Interp *interp,
		  int objc,
		  Tcl_Obj * const objv[]) 
{
    int i, j, nmap, isSpace, isPoint, nInit;
    char *tmp;
    Tcl_Obj **OMap, **OneE;
    Tk_Window tkwin = bioPtr->tkwin;
    XColor *Cfg, *Cbg;
    Tcl_Size nmapSz, jSz;

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "option ?arg arg...?");
	return TCL_ERROR;
    }

    Tcl_ListObjGetElements(interp,objv[2],&nmapSz,&OMap);
    nmap = (int)nmapSz;
  
    /* Bug fix: the previous mapping table was freed with a plain
     * ckfree() of the outer array, without ever releasing each
     * entry's own resources first (Cfg/Cbg color allocations,
     * bgBorder, gc) -- leaking all of them every time the mapping is
     * reconfigured (e.g. switching color schemes at runtime). */
    if (bioPtr->Mapping != NULL) {
	for (i=0; i<bioPtr->nbMapping; i++) {
	    if (bioPtr->Mapping[i].Cfg != NULL) {
		Tk_FreeColor(bioPtr->Mapping[i].Cfg);
	    }
	    if (bioPtr->Mapping[i].Cbg != NULL) {
		Tk_FreeColor(bioPtr->Mapping[i].Cbg);
	    }
	    if (bioPtr->Mapping[i].bgBorder != NULL) {
		Tk_Free3DBorder(bioPtr->Mapping[i].bgBorder);
	    }
	    if (bioPtr->Mapping[i].gc != NULL) {
		Tk_FreeGC(bioPtr->display, bioPtr->Mapping[i].gc);
	    }
	}
	ckfree((char *)bioPtr->Mapping);
	bioPtr->Mapping = NULL;
    }
    bioPtr->nbMapping = 0;

    if(nmap == 0) {
	return TCL_OK;
    }  

    bioPtr->Mapping = (Tag *)ckalloc(nmap*sizeof(Tag));
    memset(bioPtr->Mapping, 0, nmap*sizeof(Tag));

    /* Now read the mapping */
    /* Bug fix: keep track of how many entries were actually
     * initialized (nInit), so that any error path below can cleanly
     * release exactly the resources already allocated instead of
     * leaving a half-built table installed, or leaking. */
    nInit = 0;
    for (i=0;i<nmap;i++) {
	Tcl_ListObjGetElements(interp,OMap[i],&jSz,&OneE);
	j = (int)jSz;
	/* Bug fix: each mapping entry must have at least 3 elements
	 * (character, foreground color, background color). "j" was
	 * never checked; a malformed entry (e.g. a color forgotten by
	 * a typo) read OneE[2] out of bounds. */
	if (j < 3) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"bad mapping entry #%d: expected {char foreground background}", i));
	    goto mappingError;
	}
	tmp = Tcl_GetString(OneE[0]);
	bioPtr->Mapping[i].smap     = tmp[0];
	bioPtr->Mapping[i].bgBorder = Tk_Get3DBorder(interp, tkwin, Tcl_GetString(OneE[2]));
    
	Cfg = Tk_AllocColorFromObj(interp, tkwin, OneE[1]);
	Cbg = Tk_AllocColorFromObj(interp, tkwin, OneE[2]);
	/* Bug fix: Tk_AllocColorFromObj()/Tk_Get3DBorder() return NULL
	 * on an invalid color name; Cfg->pixel/Cbg->pixel were
	 * dereferenced unconditionally right below, crashing on any
	 * bad color name in the mapping definition. */
	if (Cfg == NULL || Cbg == NULL || bioPtr->Mapping[i].bgBorder == NULL) {
	    if (Cfg != NULL) Tk_FreeColor(Cfg);
	    if (Cbg != NULL) Tk_FreeColor(Cbg);
	    if (bioPtr->Mapping[i].bgBorder != NULL) Tk_Free3DBorder(bioPtr->Mapping[i].bgBorder);
	    goto mappingError;
	}
    
	bioPtr->Mapping[i].Cfg = Cfg;
	bioPtr->Mapping[i].Cbg = Cbg;
	bioPtr->Mapping[i].fg  = Cfg->pixel;
	bioPtr->Mapping[i].bg  = Cbg->pixel;
    
	bioPtr->Mapping[i].gc = NULL;
	nInit = i+1;
    }
    bioPtr->nbMapping = nmap;

    /*
     * check if there is a definition for ' ' and '.'
     * Otherwise add it to mapping
     */
    isPoint = GetIndexOfTag(bioPtr, ".", 0);
    isSpace = GetIndexOfTag(bioPtr, " ", 0);
    if (isSpace == -1 || isPoint == -1) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf("\nYou should define a mapping for characters ' ' and '.' !\n"));
	/* Bug fix: this used to return TCL_ERROR here while leaving
	 * bioPtr->Mapping/nbMapping fully installed with the table that
	 * was just rejected -- other code (BiotextDelcharsCmd,
	 * BiotextPushInSeq, BiotextAddColsToBufferSeqs, ...) looks up
	 * GetIndexOfTag(bioPtr, ".", 0) and stores the result straight
	 * into MapSeq[][] without checking for -1; a -1 stored there
	 * would later be used as a *negative* index into Mapping[] when
	 * drawing (Map[-1]), an out-of-bounds access. Roll back to the
	 * safe "no mapping" state instead of leaving an invalid one
	 * installed. */
	goto mappingError;
    }

#ifdef USE_XFT  
    for(i=0; i<bioPtr->nbMapping; i++) {
	Biotext_AssignColorOfTag(&(bioPtr->Mapping[i]), bioPtr->tkfont, bioPtr->tkwin, bioPtr->display);
    }  
#endif
  
    if (bioPtr->SeqMat != NULL) {
	MapSequences(bioPtr);
    }
  
    return TCL_OK;

  mappingError:
    for (i=0; i<nInit; i++) {
	if (bioPtr->Mapping[i].Cfg != NULL) {
	    Tk_FreeColor(bioPtr->Mapping[i].Cfg);
	}
	if (bioPtr->Mapping[i].Cbg != NULL) {
	    Tk_FreeColor(bioPtr->Mapping[i].Cbg);
	}
	if (bioPtr->Mapping[i].bgBorder != NULL) {
	    Tk_Free3DBorder(bioPtr->Mapping[i].bgBorder);
	}
    }
    ckfree((char *)bioPtr->Mapping);
    bioPtr->Mapping = NULL;
    bioPtr->nbMapping = 0;
    return TCL_ERROR;
}


static int 
*MapOneSequence(Biotext *bioPtr,
		char *seq)
{
    int i, k, pt, len, defaultMap;
    int *map;

    len = strlen(seq);
    map = (int *)ckalloc(len*sizeof(int));

    /* Bug fix: kept as a 2-argument function to match the prototype in
     * tkBiotext.h (this is a shared, cross-platform header, also used
     * by tkBioWin.c). The fallback index for unmapped characters (see
     * below) is now resolved internally instead of via an extra
     * parameter. GetIndexOfTag(".", 0) is guaranteed to succeed once
     * bioPtr->nbMapping > 0, since BiotextMappingCmd() validates that
     * '.' is defined before ever calling MapSequences()/this function. */
    defaultMap = GetIndexOfTag(bioPtr, ".", 0);
    if (defaultMap == -1) {
	defaultMap = 0;
    }

    for(i=0; i<len; i++) {
	pt = -1;
	for (k=0; k<bioPtr->nbMapping; k++) {
	    if (seq[i] == bioPtr->Mapping[k].smap) {
		pt = k;
		break;
	    }
	}
	/* Bug fix: a character not covered by the mapping table (the
	 * mapping is only guaranteed to define '.' and ' ', not every
	 * possible residue/symbol that might appear in a sequence) used
	 * to store -1 here. That -1 is later used as an index into
	 * Mapping[] while drawing (Map[k].fg/bg/...), an out-of-bounds
	 * (negative) array access. Fall back to a known-valid entry
	 * instead. */
	if (pt == -1) {
	    pt = defaultMap;
	}
	map[i] = pt;
    }

    return map;
}

    
static void 
MapSequences(Biotext *bioPtr)
{
    int i;

    if (bioPtr->MapSeq != NULL) {
	//fprintf(stderr, "\ndesallocate MapSeq\n");
	//fflush(stderr);
	for (i=0; i<bioPtr->nbSeqs; i++) {
	    ckfree((char *)bioPtr->MapSeq[i]);
	}
	ckfree((char *)bioPtr->MapSeq);
    }
    
    bioPtr->MapSeq = (int **)ckalloc(bioPtr->nbSeqs * sizeof(int *));
    for (i=0;i<bioPtr->nbSeqs;i++) {	
	bioPtr->MapSeq[i] = MapOneSequence(bioPtr, bioPtr->SeqMat[i]);
    }
  
    return;
}

  
/*
 *--------------------------------------------------------------
 *
 * BiotextMapCmd --
 *
 *	This procedure is invoked to process the 
 *      "map" method. It enables/disables the
 *	drawing of mapped characters. 
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None
 *
 *--------------------------------------------------------------
 */
static int 
BiotextMapCmd(Biotext *bioPtr,
	      Tcl_Interp *interp,
	      int objc,
	      Tcl_Obj * const objv[]) 
{
    int zebool, i, j;
  
    if (objc > 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "option ?arg arg...?");
	return TCL_ERROR;
    }

    /* if objc == 2 retrieve the map status */
    if (objc == 2) {
	if (bioPtr->flags & MAP_RESIDUES) {
	    zebool = 1;
	} else {
	    zebool = 0;
	}
	Tcl_SetObjResult(interp, Tcl_NewIntObj(zebool));
	return TCL_OK;
    }

    /* argument could be map on|off|clean */
    if (strcasecmp(Tcl_GetString(objv[2]), "clean") == 0) {
	/* clean mapping */
	/* Bug fix, two issues:
	 * 1) bioPtr->MapSeq is only allocated when bioPtr->nbMapping > 0;
	 *    it is NULL otherwise (the default/common state), and this
	 *    loop wrote into it unconditionally -- NULL-pointer
	 *    dereference on ".biotext map clean" whenever no mapping is
	 *    configured, same recurring bug pattern already fixed several
	 *    times elsewhere in this file (BiotextDeleteCols,
	 *    BiotextDelcharsCmd, BiotextAddColsToBufferSeqs...).
	 * 2) Every entry was set to -1, later used as an index into
	 *    Mapping[] while drawing (Map[k]...) -- an out-of-bounds
	 *    (negative) array access as soon as mapping is active and a
	 *    redraw happens. Fall back to the gap character's index
	 *    instead, same fix as MapOneSequence()/MapSequences() above. */
	if (bioPtr->nbMapping > 0 && bioPtr->MapSeq != NULL) {
	    int defaultMap = GetIndexOfTag(bioPtr, ".", 0);
	    if (defaultMap == -1) {
		defaultMap = 0;
	    }
	    for (i=0; i<bioPtr->nbSeqs; i++) {
		for (j=0; j<bioPtr->lgSeqs; j++) {
		    bioPtr->MapSeq[i][j] = defaultMap;
		}
	    }
	}
    } else {
	if (Tcl_GetBooleanFromObj(interp, objv[2], &zebool) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (zebool) {
	    /* map ON */
	    if (! (bioPtr->flags & MAP_RESIDUES)) {
		bioPtr->flags |= MAP_RESIDUES;
	    }
	} else {
	    /* map OFF */
	    if (bioPtr->flags & MAP_RESIDUES) {
		bioPtr->flags &= ~MAP_RESIDUES;
	    }
	}    
    }
  
    BiotextEventuallyRedraw(bioPtr);
  
    return TCL_OK;
}


/*
 *--------------------------------------------------------------
 *
 * BiotextTagCmd --
 *
 *	This procedure is invoked to process the 
 *      "tag" method. It dispatches the sub-commands 
 *      configure, add and remove
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None
 *
 *--------------------------------------------------------------
 */
static int 
BiotextTagCmd(Biotext *bioPtr,
	      Tcl_Interp *interp,
	      int objc,
	      Tcl_Obj * const objv[]) 
{
    int cmdIndex;
  
    static CONST84 char *tagCmdNames[] = {
	"configure", "add", "remove", "delete", "state", "names", "raise" ,"ranges", (char *)NULL
    };
    enum tagCommand {
	CMD_TAG_CONFIG, CMD_TAG_ADD, CMD_TAG_REMOVE, CMD_TAG_DELETE, CMD_TAG_STATE, CMD_TAG_NAMES, CMD_TAG_RAISE, CMD_TAG_RANGES,  
    };
  
    if (Tcl_GetIndexFromObj(interp, objv[2], tagCmdNames, "tag option", 0, &cmdIndex) != TCL_OK) {
	return TCL_ERROR;
    }

    if (strcmp(Tcl_GetString(objv[2]), "names")) {
#ifdef DEBUG_TAGCMD
	fprintf(stderr,"\n== in BiotextTagCmd ==========\n");
	fprintf(stderr, "widget %s\n", bioPtr->widgetName);
	fprintf(stderr, "sub-command : %s\n", Tcl_GetString(objv[2]));
	fprintf(stderr, "------------------\n");
	fflush(stderr);
#endif
    }
  
    switch ((enum tagCommand) cmdIndex) {
    case CMD_TAG_CONFIG:
	if (BiotextTagConfigSubCmd(bioPtr, interp, objc, objv) != TCL_OK) 
	    return TCL_ERROR;
	break;
    case CMD_TAG_ADD:
	if (BiotextTagAddSubCmd(bioPtr, interp, objc, objv) != TCL_OK) 
	    return TCL_ERROR;
	break;
    case CMD_TAG_REMOVE:
	/* remove : 
	 * we keep the tag definition but we clean display 
	 */
	if (BiotextTagRemoveSubCmd(bioPtr, interp, objc, objv) != TCL_OK) {
	    return TCL_ERROR;
	}
	break;
    case CMD_TAG_DELETE:
	/* delete tag definition and display */
	if (BiotextTagDeleteSubCmd(bioPtr, interp, objc, objv) != TCL_OK) {
	    return TCL_ERROR;
	}
	break;
    case CMD_TAG_NAMES:
	if (BiotextTagNamesSubCmd(bioPtr, interp, objc, objv) != TCL_OK) {
	    return TCL_ERROR;
	}
	break;
    case CMD_TAG_STATE:
	/* Bug fix: return value was ignored, unlike every other
	 * sub-command dispatched here. A bad boolean argument (or any
	 * other error from BiotextTagStateSubCmd) still set an error
	 * message in the interp result via Tcl_SetObjResult, but this
	 * command nonetheless returned TCL_OK -- the calling Tcl script
	 * would see the error text as if it were a (bizarre) successful
	 * return value, with no exception/catch triggered. */
	if (BiotextTagStateSubCmd(bioPtr, interp, objc, objv) != TCL_OK) {
	    return TCL_ERROR;
	}
	break;
    case CMD_TAG_RAISE:
	/* Bug fix: same ignored-return-value issue as CMD_TAG_STATE
	 * above -- BiotextTagRaiseSubCmd can fail for several real
	 * reasons (wrong arg count, no tags defined, unknown tag name)
	 * and every one of them was silently swallowed. */
	if (BiotextTagRaiseSubCmd(bioPtr, interp, objc, objv) != TCL_OK) {
	    return TCL_ERROR;
	}
	break;
    case CMD_TAG_RANGES:
	if (BiotextTagRangesSubCmd(bioPtr, interp, objc, objv) != TCL_OK) {
	    return TCL_ERROR;
	}
	break;
    }

    BiotextEventuallyRedraw(bioPtr);

    return TCL_OK;
}


/*--------------------------------------------------------------
 *
 * BiotextXViewCmd --
 *
 *	This procedure is invoked to process the 
 *      "xview" method. It update the widget in
 *	accordance to x-scrolling.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None
 *
 *--------------------------------------------------------------
 */
static int
BiotextXViewCmd(register Biotext *bioPtr,
		Tcl_Interp *interp,
		int objc,
		Tcl_Obj * const objv[]) 
{
    int index;
  
    if (objc == 2) {
	double first, last;
	Tcl_Obj *result[2];
    
	BiotextVisibleXRange(bioPtr, &first, &last);

	result[0] = Tcl_NewDoubleObj(first);
	result[1] = Tcl_NewDoubleObj(last);
	Tcl_SetObjResult(interp, Tcl_NewListObj(2, result));

	return TCL_OK;
    } else if (objc == 3) {
	/* Bug fix: this used to be an empty "TO DO" stub -- the standard
	 * Tk one-argument form "pathName xview index" (set the left
	 * column to <index>) silently did nothing at all (fell through
	 * to "return TCL_OK" without calling ChangeBiotextXView).
	 * Implemented to match the documented/standard scrolling widget
	 * convention. */
	int idx;
	if (Tcl_GetIntFromObj(interp, objv[2], &idx) != TCL_OK) {
	    return TCL_ERROR;
	}
	ChangeBiotextXView(bioPtr, idx);
    } else {
	double fraction;
	int count;
	switch (Tk_GetScrollInfoObj(interp, objc, objv, &fraction, &count)) {
	case TK_SCROLL_MOVETO:
	    index = (int) (fraction * bioPtr->lgSeqs + 0.5);
	    break;
	case TK_SCROLL_PAGES:
	    if (bioPtr->widthChar > 2) {
		index = bioPtr->leftIndex + count*(bioPtr->widthChar-2);
	    } else {
		index = bioPtr->leftIndex + count;
	    }
	    break;
	case TK_SCROLL_UNITS:
	    index = bioPtr->leftIndex + count;
	    break;
	case TK_SCROLL_ERROR:
	default:
	    return TCL_ERROR;
	}

	ChangeBiotextXView(bioPtr, index);
    }

    return TCL_OK;
}

/*--------------------------------------------------------------
 *
 * BiotextYViewCmd --
 *
 *	This procedure is invoked to process the 
 *      "yview" method. It update the widget in
 *	accordance to y-scrolling.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None
 *
 *--------------------------------------------------------------
 */
static int 
BiotextYViewCmd(register Biotext *bioPtr,
		Tcl_Interp *interp,
		int objc,
		Tcl_Obj * const objv[]) 
{
    int index;
  
    if (objc == 2) {
	double first, last;
	Tcl_Obj *result[2];
    
	BiotextVisibleYRange(bioPtr, &first, &last);

	result[0] = Tcl_NewDoubleObj(first);
	result[1] = Tcl_NewDoubleObj(last);
	Tcl_SetObjResult(interp, Tcl_NewListObj(2, result));

	return TCL_OK;
    } else if (objc == 3) {
	/* Bug fix: same missing implementation as BiotextXViewCmd above
	 * ("pathName yview index" silently did nothing). */
	int idx;
	if (Tcl_GetIntFromObj(interp, objv[2], &idx) != TCL_OK) {
	    return TCL_ERROR;
	}
	ChangeBiotextYView(bioPtr, idx);
    } else {
	double fraction;
	int count;
	switch (Tk_GetScrollInfoObj(interp, objc, objv, &fraction, &count)) {
	case TK_SCROLL_MOVETO:
	    index = (int) (fraction * bioPtr->nbSeqs + 0.5);
	    break;
	case TK_SCROLL_PAGES:
	    if (bioPtr->heightChar > 2) {
		index = bioPtr->topIndex + count*(bioPtr->heightChar-2);
	    } else {
		index = bioPtr->topIndex + count;
	    }
	    break;
	case TK_SCROLL_UNITS:
	    index = bioPtr->topIndex + count;
	    break;
	case TK_SCROLL_ERROR:
	default:
	    return TCL_ERROR;
	}
    
	ChangeBiotextYView(bioPtr, index);
    }

    return TCL_OK;
}

/*--------------------------------------------------------------
 *
 * BiotextInsertCmd --
 *
 *	This procedure is invoked to process the 
 *      "insert" method. It inserts columns of 
 *      gaps or sequences after or before index , 
 *      depending on the sign of count.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None
 *
 *--------------------------------------------------------------
 */
static int 
BiotextInsertCmd(register Biotext *bioPtr,
		 Tcl_Interp *interp,
		 int objc,
		 Tcl_Obj * const objv[]) 
{
  int i, result, count, cmdIndex;
  char *iStr, dummy;
  
  if (bioPtr->state == STATE_DISABLED) {
      return TCL_OK;
  }
  
  if (objc < 4) {
      Tcl_WrongNumArgs(interp, 1, objv, "option index ?count?");
      return TCL_ERROR;
  }
  
  static CONST84 char *insCmdNames[] = {
      "cols", "rows", (char *)NULL
  };
  enum insCommand {
      ICMD_COLS, ICMD_ROWS,
  };
  
    if (Tcl_GetIndexFromObj(interp, objv[2], insCmdNames, "insert option", 0, &cmdIndex) != TCL_OK) {
	return TCL_ERROR;
    }

    count = 1; /* default */
    if (objc == 5) {
	if (Tcl_GetInt(interp, Tcl_GetString(objv[4]), &count) != TCL_OK) {
	    return TCL_ERROR;
	}
    }

    /* Get insertion index */
    iStr = Tcl_GetString(objv[3]);
    /* Bug fix: this was "sscanf(iStr, "%d%s", &i, &dummy)". "dummy" is
     * declared as a single "char", but "%s" writes an unbounded,
     * NUL-terminated string -- any index string with more than one
     * trailing non-numeric character after the number (e.g. "5xy",
     * "5   ", or any garbage/malicious input) overflowed this 1-byte
     * stack variable. The intent here is only to detect *whether*
     * there is trailing garbage after the number (reject it if so),
     * exactly like BiotextGetIndex() already does correctly with
     * "%d.%d%c" elsewhere in this file -- "%c" reads exactly one
     * character, which is what a single "char dummy" can safely hold. */
    if (strcmp(iStr, "end") && (sscanf(iStr, "%d%c", &i, &dummy) != 1)) {
	Tcl_WrongNumArgs(interp, 1, objv, "option index ?count?");
	return TCL_ERROR;
    }
  
    switch ((enum insCommand) cmdIndex) {
    case ICMD_COLS:
	if (strcmp(iStr, "end") == 0) {
	    i = bioPtr->lgSeqs - 1;
	} else {
	    sscanf(iStr, "%d", &i);
	}
	result = BiotextInsertCols(bioPtr, i, count, -1);
	break;
    case ICMD_ROWS:
	if (strcmp(iStr, "end") == 0) {
	    i = bioPtr->nbSeqs - 1;
	} else {
	    sscanf(iStr, "%d", &i);
	}
	result = BiotextInsertRows(bioPtr, i, count);
	break;
    default:
	Tcl_WrongNumArgs(interp, 1, objv, "option index ?count?");
	result = TCL_ERROR;
    }

    if (result == TCL_OK) {
	bioPtr->flags |= UPDATE_SCROLLBAR;
	BiotextEventuallyRedraw(bioPtr);
    }

    return result;
}

/*--------------------------------------------------------------
 *
 * BiotextCharsCmd --
 *
 *      This procedure implements the "chars" method.
 *      It inserts <count> character <char> in 
 *      biotext at index <i>.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None
 *
 *--------------------------------------------------------------
 */
static int 
BiotextCharsCmd(register Biotext *bioPtr,
		Tcl_Interp *interp,
		int objc,
		Tcl_Obj * const objv[]) 
{
    int count, r, c, nGrp, i, j, ichr, nogroup, iChar;
    char *chr, *index;
    char chrBuf[2];

    if (bioPtr->state == STATE_DISABLED) {
	return TCL_OK;
    }

    if (objc < 4) {
	Tcl_WrongNumArgs(interp, 1, objv, "?-nogroup? <char> <index> ?count?");
	return TCL_ERROR;
    }
  
    /* 
     * Command : 
     * $w chars ?-nogroup? <char> <index> ?<count>?
     */
    nogroup = 0;
    iChar = 2;
    if (strcasecmp(Tcl_GetString(objv[2]), "-nogroup") == 0) {
	/* -nogroup option */
	nogroup = 1;
	iChar = 3;

	if (objc < 5) {
	    Tcl_WrongNumArgs(interp, 1, objv, "?-nogroup? <char> <index> ?count?");
	    return TCL_ERROR;
	}
    }
  
    chr = Tcl_GetString(objv[iChar]);
    if (strlen(chr) > 1) {
	return TCL_ERROR;
    }

    /* can't insert a char if matrix is locked */
    if (chr[0] != '.' && bioPtr->Lock) {
	return TCL_OK;
    }
    /* Bug fix: this used to do "chr[0] = toupper(chr[0]);", mutating
     * the Tcl_Obj's string representation bytes returned by
     * Tcl_GetString() *in place*. objv[] elements can be shared Tcl
     * objects (e.g. literal-pooled by Tcl's bytecode compiler across
     * repeated calls in a loop/proc); mutating one in place can
     * silently corrupt that shared value everywhere else it's used in
     * the script -- a classic, hard-to-diagnose Tcl pitfall. Work on a
     * private local copy instead and leave the original object alone. */
    chrBuf[0] = toupper((unsigned char)chr[0]);
    chrBuf[1] = '\0';
    chr = chrBuf;

    /* Get insertion index */
    index = Tcl_GetString(objv[iChar+1]);
    if (BiotextGetIndex(bioPtr, index, &r, &c) != TCL_OK) {
	return TCL_ERROR;
    }
  
    /* Get number of chars to insert */
    count = 1;
    if (objc == iChar+3) {
	if (Tcl_GetInt(interp, Tcl_GetString(objv[iChar+2]), &count) != TCL_OK) {
	    return TCL_ERROR;
	}
	/* Bug fix: only "count == 0" was rejected. A negative count was
	 * passed straight through to BiotextInsertCols() (which treats a
	 * negative count as a direction flag -- insert |count| columns
	 * *before* the index -- not an error), while the character
	 * placement loop below ("for (j=c; j<c+count; j++)") never runs
	 * at all when count is negative (c+count < c), so the row gets
	 * shifted using the wrong boundary but no character is ever
	 * written -- silently corrupting the row instead of crashing.
	 * "count" here is documented as a number of characters to
	 * insert, which is never meaningfully negative; treat it the
	 * same as count == 0 (a harmless no-op). */
	if (count <= 0) {
	    return TCL_OK;
	}
    }
  
    /*
     * First :
     * delete the timer of the blinking cursor
     */
    if (bioPtr->cursorTimer != NULL) {
	Tcl_DeleteTimerHandler(bioPtr->cursorTimer);
    }
  
    /* 
     * Second :
     * Insert <count> cols at the end of the alignment
     */
    if (BiotextInsertCols(bioPtr, bioPtr->lgSeqs-1, count, -1) != TCL_OK) {
	bioPtr->flags |= CURSOR_ON;
	BiotextConfigCursor(bioPtr);
	BiotextEventuallyRedraw(bioPtr);

	return TCL_ERROR;
    }
  
    /*
     * Third :
     * find tag for inserted character 
     */
    ichr = GetIndexOfTag(bioPtr, chr, 0);
    if (ichr == -1) {
	/* char not found ! */
	Tcl_SetObjResult(interp, Tcl_ObjPrintf("Character >%c< not found in mapping !", chr[0]));
	return TCL_ERROR;
    }

    /* then insert chars */
    nGrp = bioPtr->SeqGrp[r];
    if (! nogroup && nGrp != 0 && bioPtr->nbrGrps > 0) {
	for (i=0; i<bioPtr->nbSeqs; i++) {
	    if (bioPtr->SeqGrp[i] == nGrp) {
		for (j=bioPtr->lgSeqs-1; j>=c+count; j--) {
		    bioPtr->SeqMat[i][j] = bioPtr->SeqMat[i][j-count];
		    bioPtr->MapSeq[i][j] = bioPtr->MapSeq[i][j-count];
		}
		for (j=c; j<c+count; j++) {
		    bioPtr->SeqMat[i][j] = chr[0];
		    bioPtr->MapSeq[i][j] = ichr;
		}
		bioPtr->SeqMat[i][bioPtr->lgSeqs] = '\0';
	    }
	}
    } else {
	/* nogroup */
	for (j=bioPtr->lgSeqs-1; j>=c+count; j--) {
	    bioPtr->SeqMat[r][j] = bioPtr->SeqMat[r][j-count];
	    bioPtr->MapSeq[r][j] = bioPtr->MapSeq[r][j-count];
	}
	for (j=c; j<c+count; j++) {
	    bioPtr->SeqMat[r][j] = chr[0];
	    bioPtr->MapSeq[r][j] = ichr;
	}
	bioPtr->SeqMat[r][bioPtr->lgSeqs] = '\0';
    }
  
    /* move cursor +count */
    bioPtr->currentC += count;

    /* Move tags if necessary */
    if (! nogroup && nGrp != 0 && bioPtr->nbrGrps > 0) {
	char pos[64];

	for (i=0; i<bioPtr->nbSeqs; i++) {
	    if (bioPtr->SeqGrp[i] == nGrp) {
		/* Bug fix: same off-by-one as BiotextInsertCols's tag-gap
		 * loop -- "i" is 0-based, but the index string needs a
		 * 1-based row number. */
		sprintf(pos, "%d.%d", i+1, c);
		BiotextInsertGapsInTags(bioPtr, pos, count);
	    }
	}
    } else {
	BiotextInsertGapsInTags(bioPtr, index, count);
    }
  
    /* Redraw widget */
    bioPtr->flags |= UPDATE_SCROLLBAR;
    bioPtr->flags |= CURSOR_ON;
    BiotextConfigCursor(bioPtr);
    BiotextEventuallyRedraw(bioPtr);

    return TCL_OK;
}

/*--------------------------------------------------------------
 *
 * BiotextDelcharsCmd --
 *
 *      This procedure implements the "delchars" 
 *      method.
 *      It deletes <count> characters in 
 *      biotext before index <i>
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None
 *
 *--------------------------------------------------------------
 */
static int 
BiotextDelcharsCmd(register Biotext *bioPtr,
		   Tcl_Interp *interp,
		   int objc,
		   Tcl_Obj * const objv[]) 
{
    int count, r, c, nGrp, i, j, k, isgap, igap, iidx, dogrp, hasmoved, iTag, nrow, nval, start, stop, v;
    char *index;
    Tcl_Obj *itagObj, *coordsObj, *newCoordsObj, **rowObj, *tmpObj, **valObj, *newRowObj;
    Tcl_Size nrowSz, nvalSz;

    if (bioPtr->state == STATE_DISABLED) {
	return TCL_OK;
    }

    if (objc < 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "delchars index");
	return TCL_ERROR;
    }

    /* 
     * Command : $w delchars ?-group? <index> ?<count>?
     */
    if (strcmp(Tcl_GetString(objv[2]),"-group") == 0) {
	iidx = 3;
	dogrp = 1;
    } else {
	iidx  = 2;
	dogrp = 0;
    }

    index = Tcl_GetString(objv[iidx]);
    if (BiotextGetIndex(bioPtr, index, &r, &c) != TCL_OK) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf("\nindex is not valid !"));
	return TCL_ERROR;
    }

    /* leftmost edge */
    if (c == 0) {
	return TCL_OK;
    }
  
    /* count : number of chars to be deleted */
    count = 1;
    if ((! dogrp && objc == 4) || (dogrp && objc == 5)) {
	if (Tcl_GetInt(interp, Tcl_GetString(objv[objc-1]), &count) != TCL_OK) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf("\ncount is not valid !"));
	    return TCL_ERROR;
	}
    }
  
    /*
     * find tag for gap character 
     */
    igap = GetIndexOfTag(bioPtr, ".", 0);

    /*
     * First, delete the timer of the blinking cursor
     */
    if (bioPtr->cursorTimer != NULL) {
	Tcl_DeleteTimerHandler(bioPtr->cursorTimer);
    }
  
    /*
     * Loop <count> times and :
     * - check if there is a gap character at c-1
     * - if yes delete char
     * - if no, and no locking, delete char
     * - if col > 0 continue otherwise break
     */
    nGrp = bioPtr->SeqGrp[r];
    hasmoved = 0;
    for (k=0; k<count; k++) {
	if (dogrp && nGrp != 0 && bioPtr->nbrGrps > 0) {
	    /* sequence belongs to a group */
	    isgap = 1;
	    for (i=0; i<bioPtr->nbSeqs; i++) {
		if (bioPtr->SeqGrp[i] == nGrp) {
		    /* first, check for gap */
		    if (bioPtr->SeqMat[i][c-1] != '.') {
			isgap = 0;
			break;
		    }
		}
	    }
	    if (isgap || ! bioPtr->Lock) {
		hasmoved = 1;
		for (i=0; i<bioPtr->nbSeqs; i++) {
		    if (bioPtr->SeqGrp[i] == nGrp) {
			/* Bug fix: bioPtr->MapSeq is only allocated when
			 * bioPtr->nbMapping > 0 (see BiotextInitBuffer /
			 * the constructor); it is NULL otherwise, which is
			 * the common/default case. These MapSeq writes
			 * were unconditional, so deleting a character with
			 * no mapping active dereferenced a NULL pointer
			 * and crashed -- on essentially every "delchars"
			 * call in that (very common) state. */
			for (j=c-1; j<bioPtr->lgSeqs-1; j++) {
			    bioPtr->SeqMat[i][j] = bioPtr->SeqMat[i][j+1];
			    if (bioPtr->nbMapping) {
				bioPtr->MapSeq[i][j] = bioPtr->MapSeq[i][j+1];
			    }
			}
			bioPtr->SeqMat[i][bioPtr->lgSeqs-1] = '.';
			bioPtr->SeqMat[i][bioPtr->lgSeqs] = '\0';
			if (bioPtr->nbMapping) {
			    bioPtr->MapSeq[i][bioPtr->lgSeqs-1] = igap;
			}
		    }
		}
	    }
	} else {
	    /* no groups, sequence alone */
	    if (bioPtr->SeqMat[r][c-1] == '.' || ! bioPtr->Lock) {
		hasmoved = 1;
		/* Bug fix: same missing "if (bioPtr->nbMapping)" guard as
		 * the grouped-sequence branch above. */
		for (j=c-1; j<bioPtr->lgSeqs-1; j++) {
		    bioPtr->SeqMat[r][j] = bioPtr->SeqMat[r][j+1];
		    if (bioPtr->nbMapping) {
			bioPtr->MapSeq[r][j] = bioPtr->MapSeq[r][j+1];
		    }
		}
		bioPtr->SeqMat[r][bioPtr->lgSeqs-1] = '.';
		bioPtr->SeqMat[r][bioPtr->lgSeqs] = '\0';
		if (bioPtr->nbMapping) {
		    bioPtr->MapSeq[r][bioPtr->lgSeqs-1] = igap;
		}
	    }
	}
	if (! hasmoved) {
	    break;
	}
	
	/* change tag boundaries if necessary */
	if (bioPtr->nbTags > 0) {
	    for (i=0; i<bioPtr->nbTags; i++) {
		Tcl_ListObjIndex(interp, bioPtr->listTags, i, &itagObj);
		Tcl_GetIntFromObj(interp, itagObj, &iTag);

		coordsObj = Tcl_DuplicateObj(bioPtr->Tags[iTag].coords);
		newCoordsObj = Tcl_NewListObj(0, NULL);
		Tcl_ListObjGetElements(interp, coordsObj, &nrowSz, &rowObj);
		nrow = (int)nrowSz;

		for (j=0; j<nrow; j++) {
		    /* Bug fix: dead-store leak, same pattern already fixed
		     * elsewhere in this file -- the Tcl_NewListObj(0, NULL)
		     * result was immediately discarded/overwritten by
		     * Tcl_DuplicateObj just below, leaking one Tcl_Obj per
		     * tag per row per deleted character. */
		    tmpObj = Tcl_DuplicateObj(rowObj[j]);
		    Tcl_ListObjGetElements(interp, tmpObj, &nvalSz, &valObj);
		    nval = (int)nvalSz;
		    newRowObj = Tcl_NewListObj(0, NULL);
		    for (v=0; v<nval; v+=2) {
			Tcl_GetIntFromObj(interp, valObj[v], &start);
			Tcl_GetIntFromObj(interp, valObj[v+1], &stop);
			
			if (dogrp) {
			    if (nGrp != 0 && bioPtr->nbrGrps > 0 && bioPtr->SeqGrp[j] == nGrp) {
				if (start >= c) {
				    start -= 1;
				}
				if (stop >= c) {
				    stop -= 1;
				}
			    }
			} else {
			    /* not in a group */
			    if (j == r) {
				if (start >= c) {
				    start -= 1;
				}
				if (stop >= c) {
				    stop -= 1;
				}
			    }
			}

			Tcl_ListObjAppendElement(interp, newRowObj, Tcl_NewIntObj(start));
			    Tcl_ListObjAppendElement(interp, newRowObj, Tcl_NewIntObj(stop));
		    }
		    Tcl_ListObjAppendElement(interp, newCoordsObj, newRowObj);
		}
		bioPtr->Tags[iTag].coords = Tcl_DuplicateObj(newCoordsObj);
		Tcl_DecrRefCount(newCoordsObj);
	    }
	}

	/* Check if we moved. If not, stop */
	if (hasmoved) {
	    bioPtr->currentC--;
	    c--;
	    if (c == 0) {
		break;
	    }
	} else {
	    break;
	}
    }

    /* cursor goes left */
    BiotextConfigCursor(bioPtr);
    bioPtr->flags |= UPDATE_SCROLLBAR;
    BiotextEventuallyRedraw(bioPtr);

    return TCL_OK;
}


/*--------------------------------------------------------------
 *
 * BiotextLockCmd --
 *
 *      This procedure implements the "lock" method.
 *      lock = 1 avoid character deletion/insertion
 *      lock = 0 allows it.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None
 *
 *--------------------------------------------------------------
 */
static int 
BiotextLockCmd(register Biotext *bioPtr,
	       Tcl_Interp *interp,
	       int objc,
	       Tcl_Obj * const objv[]) 
{
    int lock;

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "lock <boolean>");
	return TCL_ERROR;
    }

    if (Tcl_GetBooleanFromObj(interp, objv[2], &lock) != TCL_OK) 
	return TCL_ERROR;
    bioPtr->Lock = lock;

    return TCL_OK;
}

/*--------------------------------------------------------------
 *
 * BiotextInsertCols --
 *
 *      This procedure inserts <count> columns in 
 *      biotext before/after index <i>, depending on the
 *      sign of count (<0 : left, >0 : right)
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None
 *
 *--------------------------------------------------------------
 */
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


/*--------------------------------------------------------------
 *
 * BiotextInsertRows --
 *
 *      This procedure inserts <count> rows in 
 *      biotext before/after index i, depending on the
 *      sign of count.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None
 *
 *--------------------------------------------------------------
 */
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

/*--------------------------------------------------------------
 *
 * BiotextDeleteCmd --
 *
 *	This procedure is invoked to process the 
 *      "delete" method. It deletes columns of 
 *      gaps or sequences after or before index , 
 *      depending on the sign of count.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None
 *
 *--------------------------------------------------------------
 */
static int 
BiotextDeleteCmd(register Biotext *bioPtr,
		 Tcl_Interp *interp,
		 int objc,
		 Tcl_Obj * const objv[]) 
{
  int i, ids, idf, result;
  char *what, *iStr, dummy;
  
  if (bioPtr->state == STATE_DISABLED) {
      return TCL_OK;
  }

  if (objc < 4 && (strcmp(Tcl_GetString(objv[2]),"colsgaps"))) {
      Tcl_WrongNumArgs(interp, 1, objv, " delete <colsgaps|cols|rows> ?all|end|index?");
      return TCL_ERROR;
  }

  /*
   * If there is no sequences, return
   */
  if (bioPtr->SeqMat == NULL) {
      return TCL_OK;
  }
  
  /* 
   * First check if we want to remove columns of gaps
   */
  what = Tcl_GetString(objv[2]);
  if (strcasecmp(what,"colsgaps") == 0) {
      result = BiotextDeleteColsGaps(bioPtr);
      //DumpMapping(bioPtr, "DeleteCols:");
      goto done;
  }
  
  /* Deletion according to index. Check it */
  iStr = Tcl_GetString(objv[3]);
  /* Bug fix: same single-byte stack overflow as BiotextInsertCmd and
   * BiotextAddSeqsCmd ("%d%s" writing an unbounded string into a
   * 1-byte "char dummy"). Use "%c", like BiotextGetIndex() does. */
  if (strcasecmp(iStr, "all") && strcasecmp(iStr, "end") && (sscanf(iStr, "%d%c", &i, &dummy) != 1)) {
      Tcl_WrongNumArgs(interp, 1, objv, "<colsgaps|rows|cols> ?all|end|index-list?");
      return TCL_ERROR;
  }
  
  result = TCL_OK;
  /* Bug fix (2026-10-08): "delete cols all" left i uninitialized
   * (sscanf("all", "%d") fails) and BiotextDeleteCols() shifted and
   * reallocated the rows from that random index: crash under Tcl 9
   * about one run in two (test_more.tcl). Neither BiotextDeleteCols()
   * nor BiotextDeleteRows() checks its index, so an out-of-range index
   * ("delete cols 999", "delete rows -1") read and wrote out of bounds
   * as well. Indices are now checked here. "delete rows all" deleted
   * rows 0, 1, 2... while the remaining rows shifted down, so the last
   * calls got indices past the end: rows are now deleted from the end. */
  if (strcasecmp(what,"cols") == 0) {
      if (strcasecmp(iStr, "all") == 0) {
	  Tcl_SetObjResult(interp, Tcl_NewStringObj("cannot delete all columns", -1));
	  return TCL_ERROR;
      }
      if (strcasecmp(iStr, "end") == 0) {
	  i = bioPtr->lgSeqs - 1;
      }
      if (i < 0 || i >= bioPtr->lgSeqs) {
	  Tcl_SetObjResult(interp, Tcl_ObjPrintf("bad column index \"%s\"", iStr));
	  return TCL_ERROR;
      }
      result = BiotextDeleteCols(bioPtr, i);
  } else if (strcasecmp(what, "rows") == 0) {
      if (strcasecmp(iStr, "end") == 0) {
	  ids = bioPtr->nbSeqs - 1;
	  idf = ids + 1;
      } else if (strcasecmp(iStr, "all") == 0) {
	  ids = 0;
	  idf = bioPtr->nbSeqs;
	} else {
	    ids = i;
	    idf = ids + 1;
	}
	if (ids < 0 || idf > bioPtr->nbSeqs) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf("bad row index \"%s\"", iStr));
	    return TCL_ERROR;
	}

	for (i=idf-1; i>=ids; i--) {
	    result = BiotextDeleteRows(bioPtr, i);
	}
    } else {
	// not rows,cols,colsgap so error
	Tcl_WrongNumArgs(interp, 1, objv, "delete <colsgaps|rows|cols> ?index-list?");

	return TCL_ERROR;
    }    
  
 done:
    if (result == TCL_OK) {
	bioPtr->flags |= UPDATE_SCROLLBAR;
	BiotextEventuallyRedraw(bioPtr);
    }

    return result;
}

/*--------------------------------------------------------------
 *
 * BiotextDeleteColsGaps --
 *
 *      This procedure deletes columns of gaps 
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None
 *
 *--------------------------------------------------------------
 */
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


/*--------------------------------------------------------------
 *
 * BiotextDeleteCols --
 *
 *      This procedure deletes <count> columns in 
 *      biotext before/after index i, depending on the
 *      sign of count.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None
 *
 *--------------------------------------------------------------
 */
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


/*--------------------------------------------------------------
 *
 * BiotextDeleteRows --
 *
 *      This procedure deletes the row in biotext at 
 *      the given index position.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None
 *
 *--------------------------------------------------------------
 */
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


/*--------------------------------------------------------------
 *
 * BiotextGroupCmd --
 *
 *      This routine handles the "group" method.
 *      This routine groups sequences given as a 
 *      list. Subsequent action on any sequence of 
 *      group will affect all sequences of the group.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None
 *
 *--------------------------------------------------------------
 */
static int 
BiotextGroupCmd(register Biotext *bioPtr,
		Tcl_Interp *interp,
		int objc,
		Tcl_Obj * const objv[]) 
{
    int nseq, i, idx, nmGrp, inm, isq, mode;
    Tcl_Size nseqSz;
    char *smode;
    Tcl_Obj **OIdx;
  
    /*
     * Syntax: <pathname> group ?-new|-merge? name index
     *
     * mode = 1 : 
     * By default, the mode of group creation is "new".
     * If sequences to be grouped already belong to a 
     * present group, they are discared from that 
     * group
     * mode = 0 :
     * Sequences are merged 
     */
    mode = 1;
    if (objc == 4) {
	inm = 2;
	isq = 3;
    } else if (objc == 5) {
	smode = Tcl_GetString(objv[2]);
	if (strcmp(smode, "-new") && strcmp(smode, "-merge")) {
	    return TCL_ERROR;
	}
	if (strcmp(smode, "-new") == 0) {
	    mode = 1;
	} else {
	    mode = 0;
	}
	inm = 3;
	isq = 4;
    } else {
	Tcl_WrongNumArgs(interp, 1, objv, "group ?-merge|-new? <group-index> <index-list>");
	return TCL_ERROR;
    }

    if (Tcl_GetIntFromObj(interp, objv[inm], &nmGrp) != TCL_OK) {
	/* Bug fix: return value was never checked; on a non-integer
	 * group name/id, nmGrp was left uninitialized (garbage stack
	 * value) and used below regardless. */
	return TCL_ERROR;
    }
    if (nmGrp == 0) return TCL_OK;
    if (mode) {
	if (bioPtr->GroupList == NULL) {
	    bioPtr->GroupList = Tcl_NewListObj(0, NULL);
	}
	Tcl_ListObjAppendElement(interp, bioPtr->GroupList, Tcl_NewIntObj(nmGrp));
	bioPtr->nbrGrps++;
    
	Tcl_ListObjGetElements(interp, objv[isq], &nseqSz, &OIdx);
	nseq = (int)nseqSz;
	for (i=0;i<nseq;i++) {
	    Tcl_GetIntFromObj(interp, OIdx[i], &idx);
	    /* Bug fix: idx (an arbitrary index coming straight from the
	     * Tcl caller) was never bounds-checked before being used to
	     * write into bioPtr->SeqGrp[] -- an out-of-range index (a
	     * stale index after some rows were deleted, or simply a
	     * scripting mistake) caused an out-of-bounds write. */
	    if (idx < 0 || idx >= bioPtr->nbSeqs) {
		continue;
	    }
	    bioPtr->SeqGrp[idx] = nmGrp;
	}
    } else {
	/* Bug fix: this "-merge" branch used to be entirely empty --
	 * ".biotext group -merge name index-list" silently did nothing
	 * at all: no sequence was ever added to the group. Implemented
	 * to match the documented behavior ("mode = 0: sequences are
	 * merged [into the existing group]"): assign the given
	 * sequences to group "nmGrp" without touching GroupList/nbrGrps,
	 * since -merge targets a group that is expected to already
	 * exist (created earlier via -new), unlike -new which creates
	 * a fresh group entry. */
	Tcl_ListObjGetElements(interp, objv[isq], &nseqSz, &OIdx);
	nseq = (int)nseqSz;
	for (i=0;i<nseq;i++) {
	    Tcl_GetIntFromObj(interp, OIdx[i], &idx);
	    if (idx < 0 || idx >= bioPtr->nbSeqs) {
		continue;
	    }
	    bioPtr->SeqGrp[idx] = nmGrp;
	}
    }

    return TCL_OK;
}


/*--------------------------------------------------------------
 *
 * BiotextUngroupCmd --
 *
 *      This procedure ungroups sequences given as an 
 *      index list. 
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None
 *
 *--------------------------------------------------------------
 */
static int 
BiotextUngroupCmd(register Biotext *bioPtr,
		  Tcl_Interp *interp,
		  int objc,
		  Tcl_Obj *const objv[]) 
{
    int nseq, i, j, idx, val, isIn;
    Tcl_Size nseqSz;
    Tcl_Obj **OIdx, *el;

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "ungroup <index-list>");
	return TCL_ERROR;
    }
  
    /*
     * !! GROUPE NAME GERE PAR LA COUCHE TCL !!!
     *
     * Find a group name. Loop through existing names 
     * and find the first missing one
     */

    /*
      isIn = 1;
      nmGrp = 0;
      while (isIn) {
      nmGrp++;
      isIn = 0;
      for(i=0,i<bioPtr->nbrGrps;i++) {
      Tcl_ListObjIndex(interp, bioPtr->GroupList, i, &el);
      Tcl_GetIntFromObj(interp, el, &val);
      if (nmGrp == val) isIn = 1;
      }
      }
    */

    Tcl_ListObjGetElements(interp,objv[2],&nseqSz,&OIdx);
    nseq = (int)nseqSz;
    for (i=0;i<nseq;i++) {
	Tcl_GetIntFromObj(interp, OIdx[i], &idx);
	/* Bug fix: same missing bounds-check as BiotextGroupCmd -- an
	 * out-of-range idx caused an out-of-bounds write into
	 * bioPtr->SeqGrp[]. */
	if (idx < 0 || idx >= bioPtr->nbSeqs) {
	    continue;
	}
	bioPtr->SeqGrp[idx] = 0;
    }

    /*
     * Check if a group is empty, or contains only one 
     * sequence. Beware, Sequences may be in the buffer.
     */
    /* Bug fix: this loop calls BiotextDeleteGroup() below, which
     * removes an element from bioPtr->GroupList and decrements
     * bioPtr->nbrGrps -- both the very list and count this loop
     * iterates over. Walking forward (i=0..nbrGrps-1, re-reading the
     * live, shrinking nbrGrps as the loop bound) meant that after any
     * group got deleted, the group that shifted into the
     * just-vacated position was skipped entirely: if two or more
     * groups became empty in the same "ungroup" call, only every
     * other one actually got cleaned up, leaving stale group entries
     * behind. Iterating backwards is immune to this, since removing
     * an entry only ever shifts positions after it (all of which have
     * already been visited when going backwards). */
    for(i=bioPtr->nbrGrps-1; i>=0; i--) {
	Tcl_ListObjIndex(interp, bioPtr->GroupList, i, &el);
	Tcl_GetIntFromObj(interp, el, &val);
	isIn = 0;
	for (j=0;j<bioPtr->nbSeqs;j++) {
	    if (bioPtr->SeqGrp[j] == val) {
		isIn++;
	    }
	}
	if (isIn < 2 && bioPtr->nbSeqsCache > 0) {
	    for(j=0;j<bioPtr->nbSeqsCache;j++) {
		if (bioPtr->SeqGrpCache[j] == val) {
		    isIn++;
		}
	    }
	}
	if (isIn < 2) 
	    BiotextDeleteGroup(bioPtr, val);
    }

    return TCL_OK;
}

/*--------------------------------------------------------------
 *
 * BiotextCursorCmd --
 *
 *      This procedure implements the "cursor" mathod.
 *      It sets or gives the cursor position or 
 *      set it to the given index.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None
 *
 *--------------------------------------------------------------
 */
static int 
BiotextCursorCmd(register Biotext *bioPtr,
		 Tcl_Interp *interp,
		 int objc,
		 Tcl_Obj * const objv[]) 
{
    int r, c;
    char *index = NULL;
    char bufr[INDEX_BUFSIZE], bufc[INDEX_BUFSIZE];
  
    if (objc > 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "index  ?index?");
	return TCL_ERROR;
    }
  
    if (objc == 3) {
	index = Tcl_GetString(objv[2]);
	/* Bug fix: BiotextGetIndex()'s return value was never checked.
	 * On a malformed index it leaves r/c uninitialized (garbage
	 * stack values) -- and unlike other call sites, this garbage
	 * was then stored persistently into bioPtr->currentR/currentC,
	 * the widget's current cursor position. That state is read
	 * by many other functions afterward (BiotextPushCmd indexes
	 * bioPtr->SeqGrp[bioPtr->currentR] directly, BiotextDelcharsCmd,
	 * rendering, ...) without re-validating it, so a single bad
	 * "cursor" call could cause out-of-bounds access much later,
	 * far from this call site. */
	if (BiotextGetIndex(bioPtr, index, &r, &c) != TCL_OK) {
	    return TCL_ERROR;
	}
#ifdef DEBUG_CURSORCMD
	fprintf(stderr, "\n== in BiotextCursorCmd ==========\n");
	fprintf(stderr, "widget %s\n", bioPtr->widgetName);
	fprintf(stderr, "index %s :\noldR %d oldC %d r %d c %d\n", index, bioPtr->currentR, bioPtr->currentC, r, c);
	fflush(stderr);
#endif
    
	bioPtr->currentR = r;
	bioPtr->currentC = c;
    } else {
	/* for output,
	   add 1 to row index as it starts at 1
	*/
#ifdef DEBUG_CURSORCMD
	fprintf(stderr,"output cursorCmd:\nr= %d c= %d\n", bioPtr->currentR, bioPtr->currentC);
	fflush(stderr);
#endif
    
	sprintf(bufr,"%d",bioPtr->currentR+1);
	sprintf(bufc,"%d",bioPtr->currentC);
    
	Tcl_SetResult(interp, bufr, NULL);
	Tcl_AppendResult(interp,".",bufc, (char *)NULL);
    }

    BiotextEventuallyRedraw(bioPtr);

    return TCL_OK;
}


/*--------------------------------------------------------------
 *
 * BiotextIndexCmd --
 *
 *      This procedure implements the "index" method.
 *      It gives the row and col coordinates 
 *      equivalent to the index given as argument.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None
 *
 *--------------------------------------------------------------
 */
static int 
BiotextIndexCmd(register Biotext *bioPtr,
		Tcl_Interp *interp,
		int objc,
		Tcl_Obj * const objv[]) 
{
    char *index = NULL;
    char bufr[INDEX_BUFSIZE], bufc[INDEX_BUFSIZE];
    int r, c;
  
    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "index  ?index?");
	return TCL_ERROR;
    }
  
    index = Tcl_GetString(objv[2]);
    /* Bug fix: same missing return-value check as BiotextCursorCmd
     * above -- r/c could be left uninitialized on a malformed index. */
    if (BiotextGetIndex(bioPtr, index, &r, &c) != TCL_OK) {
	return TCL_ERROR;
    }
    r += 1;
  
    /* for output,
       add 1 to row index as it starts at 1
    */
    sprintf(bufr,"%d",r);
    sprintf(bufc,"%d",c);
  
    Tcl_SetResult(interp, bufr, NULL);
    Tcl_AppendResult(interp,".",bufc, (char *)NULL);
  
    return TCL_OK;
}


/*--------------------------------------------------------------
 *
 * BiotextPushCmd --
 *
 *      This procedure handles the "push" method.
 *      The sequence(s) at current index are pushed 
 *      to the left or right up to the next gap.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None
 *
 *--------------------------------------------------------------
 */
static int 
BiotextPushCmd(register Biotext *bioPtr,
	       Tcl_Interp *interp,
	       int objc,
	       Tcl_Obj * const objv[]) 
{
    int r, c, left, nextGap, i, icol, nGrp;
    char *what;

    if (bioPtr->state == STATE_DISABLED) {
	return TCL_OK;
    }

    if (objc < 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "push left|right <index>");
	return TCL_ERROR;
    }

    what = Tcl_GetString(objv[2]);
    Tcl_UtfToLower(what);
    if (strncmp(what, "left", 4) != 0 && strncmp(what, "right", 5) != 0) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf("Error ! Push command argument can only be left or right !\n"));
	return TCL_ERROR;
    }
    left = 0;
    if (strcasecmp(what, "left") == 0) {
	left = 1;
    }

    /*
     * Get index :
     * <r> is the index of the reference sequence
     */
    r = bioPtr->currentR;
    c = bioPtr->currentC;
    icol = c;
  
    /* 
     * Find the closest gap to the left/right
     *
     * Is the sequence in a group ? 
     */
    nGrp = bioPtr->SeqGrp[r];
    if (nGrp != 0 && bioPtr->nbrGrps > 0) {
	nextGap = BiotextFindNextGapOfGroup(bioPtr, r, nGrp, c, left);
    } else {
	nextGap = BiotextFindNextGap(bioPtr, r, c, left);
    }
    //DumpSeqs(bioPtr, "entre PushCmd");
    //DumpCoordsTag(bioPtr, "entre PushCmd");
    /*
     * if nextGap == -1/end, we are at the beginning/end 
     * of the alignment, so insert a gap column before 
     * col 0 or after last col.
     * Remove the gap at nextGap, push sequences
     * and insert gap at c.
     */
#if 0
    fprintf(stderr, "\ndebug:\nnGrp %d r= %d col= %d nextGap= %d\n", nGrp, r, col, nextGap);
    fflush(stderr);
#endif
    if (nextGap == -1) {
	BiotextInsertCols(bioPtr, 0, -1, -1);
	nextGap = 0;

	/* 
	 * If we insert cols to the left, the "current"
	 * and icol positions should be increased by 1 
	 */
	bioPtr->currentC++;
	icol++;
    } else if (nextGap == bioPtr->lgSeqs) {
	BiotextInsertCols(bioPtr, bioPtr->lgSeqs-1, 1, -1);
	nextGap = bioPtr->lgSeqs-1;
	//DumpNullChar(bioPtr);
    }
    //DumpSeqs(bioPtr, "dans Push, apres insertCold");
    /* 
     * We can then push the sequence
     */
    if (nGrp != 0 && bioPtr->nbrGrps > 0) {
	for(i=0; i<bioPtr->nbSeqs; i++) 
	    if (bioPtr->SeqGrp[i] == nGrp)  {
		BiotextPushInSeq(bioPtr, i, icol, nextGap);
	    }
    } else {
	BiotextPushInSeq(bioPtr, r, icol, nextGap);
    }
    //DumpSeqs(bioPtr, "dans Push apres pushinseq");
    if (left) {
	bioPtr->currentC--;
    } else {
	bioPtr->currentC++;
    }

    /* we have to update the scrollbars */
    bioPtr->flags |= UPDATE_SCROLLBAR;
    BiotextEventuallyRedraw(bioPtr);
  
    return TCL_OK;
}

/*--------------------------------------------------------------
 *
 * BiotextSeeCmd --
 *
 *      This procedure handles the "see" method.
 *      The biotext widget adjusts the view in order 
 *      to make the index position visible.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None
 *
 *--------------------------------------------------------------
 */
static int 
BiotextSeeCmd(register Biotext *bioPtr,
	      Tcl_Interp *interp,
	      int objc,
	      Tcl_Obj * const objv[]) 
{
    int result, row, col, firstC, lastC, firstR, lastR;
    int width, height, firstX, lastX, firstY, lastY;
    int visible, same, totalX, totalY;
    char *index;
    char firstStr[TCL_DOUBLE_SPACE+1], lastStr[TCL_DOUBLE_SPACE+1];
    double min, max;
  
    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "index");
	return TCL_ERROR;
    }
  
    index = Tcl_GetString(objv[2]);
    /* Bug fix: missing return-value check, same pattern as the other
     * BiotextGetIndex() call sites. Not a memory-safety issue here
     * specifically since row/col get defensively clamped into range
     * right below regardless, but a bad index should be reported as
     * an error rather than silently clamped from garbage. */
    if (BiotextGetIndex(bioPtr, index, &row, &col) != TCL_OK) {
	return TCL_ERROR;
    }
#ifdef DEBUG_SEECMD
    fprintf(stderr, "\n== in BiotextSeeCmd ==========\n");
    fprintf(stderr, "widget %s\n", bioPtr->widgetName);
    fprintf(stderr, "index : %s\n  currR %d currC %d r %d c %d\n", index, bioPtr->currentR, bioPtr->currentC, row, col);
    fprintf(stderr,"charHeight %d lineHeight %d",bioPtr->charHeight, bioPtr->lineHeight);
    fflush(stderr);
#endif
  
    /* boundaries */
    width  = bioPtr->widthChar;
    height = bioPtr->heightChar;
    row    = MAX(0,row);
    row    = MIN(row,bioPtr->nbSeqs-1);
    col    = MAX(0,col);
    col    = MIN(col,bioPtr->lgSeqs-1);

    /* if index is already visible, return */
    visible = Biotext_IsVisible(bioPtr, col, row, 1);
#ifdef DEBUG_SEECMD
    fprintf(stderr,"  IsVisible %d for row %d col %d\n", visible, row, col);
    fflush(stderr);
#endif
  
    if (! visible) {
	totalX  = bioPtr->lgSeqs*bioPtr->charWidth + 2 * bioPtr->inset;
	totalY  = bioPtr->nbSeqs * bioPtr->charHeight + 2 * bioPtr->inset;
	same    = (row == bioPtr->currentR) && (col == bioPtr->currentC);

	if (same || (row != bioPtr->currentR)) {
	    /* Update only y-scrollbar */
	    firstC = col;
	    lastC  = col;
	    if ((row-height/2) < 0) {
		firstR = 0;
		lastR  = height - 1;
		min = 0;
		Biotext_Coords2Pixels(bioPtr, lastC, lastR, &lastX, &lastY, 0);
		max = (double)lastY/totalY;
	    } else if (row > bioPtr->nbSeqs-1-height/2) {
		firstR = bioPtr->nbSeqs - height;
		lastR  = bioPtr->nbSeqs - 1;
		max    = 1.0;
		Biotext_Coords2Pixels(bioPtr, firstC, firstR, &firstX, &firstY, 0);
		min    = (double)firstY/totalY;
	    } else {
		firstR = row-height/2;
		lastR  = firstR + height - 1;
		Biotext_Coords2Pixels(bioPtr, firstC, firstR, &firstX, &firstY, 0);
		min    = (double)firstY/totalY;
		Biotext_Coords2Pixels(bioPtr, lastC, lastR, &lastX, &lastY, 0);
		max    = (double)lastY/totalY;
	    }
      
	    /*
	     * Send X and Y first and last 
	     * positions to the scrollbars.
	     */
	    Tcl_Preserve(interp);
      
	    /* Y - scrollbar */
	    firstStr[0] = lastStr[0] = ' ';
	    Tcl_PrintDouble(NULL, min, firstStr+1);
	    Tcl_PrintDouble(NULL, max, lastStr+1);
      
	    result = Tcl_VarEval(interp, bioPtr->yscrollCmd, firstStr, lastStr, NULL);
	    if (result != TCL_OK) {
		Tcl_AddErrorInfo(interp, "\n    (vertical scrolling command executed by biotext)");
		Tcl_BackgroundError(interp);
	    }
      
	    bioPtr->topIndex  = firstR;
	    Tcl_Release(interp);
	}
    
	if (same || (col != bioPtr->currentC)) {
	    /* Update only x-scrollbar */
	    firstR = row;
	    lastR  = row;
	    if (col-width/2 < 0) {
		firstC = 0;
		lastC  = width - 1;
		min = 0.0;
		Biotext_Coords2Pixels(bioPtr, lastC, lastR, &lastX, &lastY, 0);
		max = (double)lastX/totalX;
	    } else if (col > bioPtr->lgSeqs-1-width/2) {
		firstC = bioPtr->lgSeqs - width;
		lastC  = bioPtr->lgSeqs - 1;
		max = 1.0;
		Biotext_Coords2Pixels(bioPtr, firstC, firstR, &firstX, &firstY, 0);
		min = (double)firstX/totalX;
	    } else {
		firstC = col-width/2;
		lastC  = firstC + width;
		Biotext_Coords2Pixels(bioPtr, firstC, firstR, &firstX, &firstY, 0);
		min = (double)firstX/totalX;
		Biotext_Coords2Pixels(bioPtr, lastC, lastR, &lastX, &lastY, 0);
		max = (double)lastX/totalX;
	    }
      
	    bioPtr->leftIndex = firstC;
      
	    /*
	     * Send X and Y first and last 
	     * fractional position to the
	     * scrollbars.
	     */
	    Tcl_Preserve(interp);
      
	    /* X - scrollbar */
	    firstStr[0] = lastStr[0] = ' ';
	    Tcl_PrintDouble(NULL, min, firstStr+1);
	    Tcl_PrintDouble(NULL, max, lastStr+1);
      
	    result = Tcl_VarEval(interp, bioPtr->xscrollCmd, firstStr, lastStr, NULL);
	    if (result != TCL_OK) {
		Tcl_AddErrorInfo(interp, "\n    (horizontal scrolling command executed by biotext)");
		Tcl_BackgroundError(interp);
	    }
      
	    Tcl_Release(interp);
	}
	bioPtr->currentC = col;
	bioPtr->currentR = row;
    }

#ifdef DEBUG_SEECMD  
    fprintf(stderr,"  output :\nrow= %d col= %d", row, col);
    fprintf(stderr,"\n------------------\n");
    fflush(stderr);
#endif
  
    BiotextConfigCursor(bioPtr);
    bioPtr->flags |= UPDATE_SCROLLBAR;
    BiotextEventuallyRedraw(bioPtr);

    return TCL_OK;
}

/*--------------------------------------------------------------
 *
 * BiotextCopyCmd --
 *
 *      This procedure handles the "copy" method.
 *      The sequences corresponding to the indices
 *      given are copied to the cut/copy/paste buffer.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None
 *
 *--------------------------------------------------------------
 */
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


/*--------------------------------------------------------------
 *
 * BiotextCutCmd --
 *
 *      This procedure handles the "cut" method.
 *      The sequences corresponding to the indices
 *      given are cut from the widget, and stored in
 *      memory.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None
 *
 *--------------------------------------------------------------
 */
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


/*--------------------------------------------------------------
 *
 * BiotextPasteCmd --
 *
 *      This procedure handles the "paste" method.
 *      The sequences in the paste buffer are copied 
 *      back inside the widget after the given index.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None
 *
 *--------------------------------------------------------------
 */
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

/*--------------------------------------------------------------
 *
 * BiotextFontCmd --
 *
 *      This procedure handles the "font" method.
 *      This routine sets a new font, or increase or
 *      decrease the size of the current font.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None
 *
 *--------------------------------------------------------------
 */
static int 
BiotextFontCmd(register Biotext *bioPtr,
	       Tcl_Interp *interp,
	       int objc,
	       Tcl_Obj * const objv[]) 
{
    char *what;
    
    if (objc != 3 && objc != 4 && objc != 6 && objc != 8) {
	Tcl_WrongNumArgs(interp, 2, objv, "?family <family>? ?size <size>? ?weight <weight>? | <bigger|smaller>");
	return TCL_ERROR;
    }

    static CONST84 char *fontCmdNames[] = {
	"family", "size", "weight", (char *)NULL
    };
    enum fontCommand {
	CMD_FFAMILY, CMD_FSIZE, CMD_FWEIGHT
    };

    if (objc == 3) {
	what = Tcl_GetString(objv[2]);

	if (strcmp(what, "metrics") == 0) {
	    Tcl_Obj *resObj;
	    resObj = Tcl_NewListObj(0, NULL);

#if USE_XFT
	    Tcl_ListObjAppendElement(interp, resObj, Tcl_NewStringObj("-ascent",-1));
	    Tcl_ListObjAppendElement(interp, resObj, Tcl_NewIntObj(bioPtr->ascent));
	    Tcl_ListObjAppendElement(interp, resObj, Tcl_NewStringObj("-descent",-1));
	    Tcl_ListObjAppendElement(interp, resObj, Tcl_NewIntObj(bioPtr->descent));
	    Tcl_ListObjAppendElement(interp, resObj, Tcl_NewStringObj("-linespace",-1));
	    Tcl_ListObjAppendElement(interp, resObj, Tcl_NewIntObj(bioPtr->charHeight));
	    Tcl_ListObjAppendElement(interp, resObj, Tcl_NewStringObj("-fixed",-1));
	    Tcl_ListObjAppendElement(interp, resObj, Tcl_NewIntObj(-1));
	    Tcl_ListObjAppendElement(interp, resObj, Tcl_NewStringObj("-width",-1));
	    Tcl_ListObjAppendElement(interp, resObj, Tcl_NewIntObj(bioPtr->charWidth));

	    Tcl_ResetResult(interp);
	    Tcl_SetObjResult(interp, resObj);

#else
	    Tcl_DString scr;

	    Tcl_DStringInit(&scr);
	    //Tcl_DStringAppend(&scr, "return [font metrics [list \"", -1);
	    Tcl_DStringAppend(&scr, "font metrics [list \"", -1);
	    Tcl_DStringAppend(&scr, bioPtr->FontFamily, -1);
	    Tcl_DStringAppend(&scr, "\" ", -1);
	    char buf[10];
	    sprintf(buf, "%d", bioPtr->FontSize);
	    Tcl_DStringAppend(&scr, buf, -1);
	    //Tcl_DStringAppend(&scr, "]]", -1);
	    Tcl_DStringAppend(&scr, "]", -1);
	    //fprintf(stderr, "\nscript:\n%s\n", Tcl_DStringValue(&scr));
	    //fflush(stderr);
	    int code;
	    code = Tcl_EvalEx(interp,Tcl_DStringValue(&scr), -1, TCL_EVAL_GLOBAL);
	    if (code != TCL_OK) {
		Tcl_Obj *options = Tcl_GetReturnOptions(interp, code);
		Tcl_Obj *key = Tcl_NewStringObj("-errorinfo", -1);
		Tcl_Obj *stackTrace;
		Tcl_IncrRefCount(key);
		Tcl_DictObjGet(NULL, options, key, &stackTrace);
		Tcl_DecrRefCount(key);
		//fprintf(stderr, "\nstack: %s\n", Tcl_GetString(stackTrace));
	    }
	    Tcl_DStringFree(&scr);
	    memset(buf, 0, sizeof(buf));
	    
	    Tcl_Obj *resInterp;
	    resInterp = Tcl_GetObjResult(interp);
	    Tcl_Size len;
	    Tcl_ListObjLength(interp, resInterp, &len);
	    //fprintf(stderr, "length %d\n", len);
	    //fprintf(stderr, "\nresult: %s\n", Tcl_GetString(resInterp));
	    //fflush(stderr);
	    //lulu
	    Tcl_Obj *elt;
	    for (int i=0 ; i< len; i++) {
		Tcl_ListObjIndex(interp, resInterp, i, &elt);
		Tcl_ListObjAppendElement(interp, resObj, elt);
	    }


	    Tcl_ResetResult(interp);

	    Tcl_DStringInit(&scr);
	    //Tcl_DStringAppend(&scr, "return [font measure [list \"", -1);
	    Tcl_DStringAppend(&scr, "font measure [list \"", -1);
	    Tcl_DStringAppend(&scr, bioPtr->FontFamily, -1);
	    Tcl_DStringAppend(&scr, "\" ",-1);
	    sprintf(buf, "%d", bioPtr->FontSize);
	    Tcl_DStringAppend(&scr, buf, -1);
	    Tcl_DStringAppend(&scr, "] \"M\"", -1);
	    //fprintf(stderr, "\nscript:\n%s\n", Tcl_DStringValue(&scr));
	    //fflush(stderr);
	    if (Tcl_EvalEx(interp,Tcl_DStringValue(&scr), -1, TCL_EVAL_GLOBAL) != TCL_OK) {
		//fprintf(stderr,"failed big script !\n");
	    }
	    Tcl_DStringFree(&scr);

	    Tcl_ListObjAppendElement(interp, resObj, Tcl_NewStringObj("-width", -1));
	    Tcl_Obj *resObj2;
	    resObj2 = Tcl_GetObjResult(interp);
	    Tcl_ListObjAppendElement(interp, resObj, resObj2);
	    Tcl_ResetResult(interp);
	    Tcl_SetObjResult(interp, resObj);
	    #endif

	    return TCL_OK;
	}

	    if (strcmp(what, "list") == 0) {
	    Tcl_Obj *fontList;
	    fontList = Tcl_NewListObj(0, NULL);

#if USE_XFT
	    /* Build pattern: monospace fonts only */
	    FcPattern   *pattern = FcPatternCreate();
	    FcObjectSet *os      = FcObjectSetBuild(FC_FAMILY, FC_SPACING, NULL);
	    
	    if (!pattern || !os) {
		Tcl_SetResult(interp, "fontconfig: out of memory", TCL_STATIC);
		if (pattern) FcPatternDestroy(pattern);
		if (os)      FcObjectSetDestroy(os);
		return TCL_ERROR;
	    }

	    FcPatternAddInteger(pattern, FC_SPACING, FC_MONO);

	    FcFontSet *fs = FcFontList(NULL, pattern, os);
	    FcPatternDestroy(pattern);
	    FcObjectSetDestroy(os);
	    
	    if (!fs) {
		Tcl_SetResult(interp, "fontconfig: FcFontList failed", TCL_STATIC);
		return TCL_ERROR;
	    }

	    /* Deduplicate and collect family names */
	    const int   max_fonts = 1024;
	    char       *families[max_fonts];
	    int         nfamilies = 0;
	    int spacing, is_mono, screen = Tk_ScreenNumber(bioPtr->tkwin);
	    Display *display = bioPtr->display;
	    
	    for (int i = 0; i < fs->nfont && nfamilies < max_fonts; i++) {
		FcChar8 *family = NULL;
		if (FcPatternGetString(fs->fonts[i], FC_FAMILY, 0, &family) != FcResultMatch) {
		    continue;
		}
		spacing = -1;
		FcPatternGetInteger(fs->fonts[i], FC_SPACING, 0, &spacing);
		
		if (spacing == FC_MONO || spacing == FC_DUAL) {
		    is_mono = 1;                  /* trust the metadata */
		} else if (spacing == FC_PROPORTIONAL) {
		    is_mono = 0;                  /* explicitly proportional, skip */
		} else {
		    /* metadata missing — measure glyphs */
		    is_mono = FontIsMonospace(display, screen, (char *)family, 16);
		}
		
		if (! is_mono) {
		    continue;
		}

		/* Skip if already seen */
		if (FontAlreadySeen((char *)family, families, nfamilies))
		    continue;

		families[nfamilies] = strdup((char *)family);
		if (!families[nfamilies]) {          /* strdup OOM */
		    Tcl_SetResult(interp, "out of memory", TCL_STATIC);
		    goto cleanup;
		}

		Tcl_ListObjAppendElement(interp, fontList, Tcl_NewStringObj((char *)family, -1));
		nfamilies++;
	    }

	    /* Success path */
	    FcFontSetDestroy(fs);
	    for (int i = 0; i < nfamilies; i++) free(families[i]);

	    Tcl_SetObjResult(interp, fontList);
	    return TCL_OK;

	    cleanup:
	    FcFontSetDestroy(fs);
	    for (int i = 0; i < nfamilies; i++) free(families[i]);
	    Tcl_DecrRefCount(fontList);
	    return TCL_ERROR;
#else
	    if (Tcl_EvalEx(interp,"set res {} ; foreach f [font families] { if {[font metrics [list -family $f] -fixed]} {lappend res $f } } ; return $res", -1, TCL_EVAL_GLOBAL) != TCL_OK) {
		//fprintf(stderr,"failed big script !\n");
	    }
	    fontList = Tcl_GetObjResult(interp);
	    Tcl_ResetResult(interp);
	    Tcl_SetObjResult(interp, fontList);
		
	    return TCL_OK;
#endif
	}

	if (strcmp(what, "bigger") == 0) { 
	    bioPtr->FontSize++;
	}
	    if (strcmp(what, "smaller") == 0) {
	    if (bioPtr->FontSize > 4) {
		bioPtr->FontSize--;
	    }
	}

	if (strcmp(what, "XLFD") == 0) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(bioPtr->FontXlfd, -1));
	    return TCL_OK;
	}

	/* Bug fix: "family"/"size"/"weight" with no value (the getter
	 * form, e.g. ".b font family") matched none of the cases above
	 * and simply fell through to "goto done" below, which never sets
	 * an explicit Tcl result -- so the command silently returned
	 * whatever was already in the interpreter's result (typically
	 * empty), instead of the actual current family/size/weight, and
	 * without ever reporting an error either. This is what made it
	 * impossible to verify what font settings are actually in effect
	 * (needed to investigate/regression-test the "default appears
	 * bold" report). Implemented proper getters, matching how
	 * "-family"/"-fixed"/"-width" are reported inside "metrics". */
	if (strcmp(what, "family") == 0) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(bioPtr->FontFamily, -1));
	    return TCL_OK;
	}
	if (strcmp(what, "size") == 0) {
	    Tcl_SetObjResult(interp, Tcl_NewIntObj(bioPtr->FontSize));
	    return TCL_OK;
	}
	if (strcmp(what, "weight") == 0) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(bioPtr->FontWeight, -1));
	    return TCL_OK;
	}

	goto done;
    }
  
    int cmdIndex, i, size, result;
    char *fam, *wgt;
  
    for (i=2;i<objc;i++) {
	if (Tcl_GetIndexFromObj(interp, objv[i], fontCmdNames, "font option", 0, &cmdIndex) != TCL_OK) {
	    return TCL_ERROR;
	}
    
	switch ((enum fontCommand) cmdIndex) {
	case CMD_FFAMILY:
	    i++;
	    fam = Tcl_GetString(objv[i]);
	    if (bioPtr->FontFamily != NULL) {
		ckfree((char *)bioPtr->FontFamily);
	    }
	    bioPtr->FontFamily = (char *)ckalloc((strlen(fam)+1)*sizeof(char));
	    strcpy(bioPtr->FontFamily, fam);
	    break;
	case CMD_FSIZE:
	    i++;
	    what = Tcl_GetString(objv[i]);
	    result = Tcl_GetInt(interp, what, &size);
	    if (result != TCL_OK) {
		Tcl_AddErrorInfo(interp, ": Font size must be an interger");
		Tcl_BackgroundError(interp);
	    } else {
		if (size < 4) size = 4;
		bioPtr->FontSize = size;
	    }
	    break;
	case CMD_FWEIGHT:
	    i++;
	    wgt = Tcl_GetString(objv[i]);
	    {
		/* Bug fix: this used to call Tcl_UtfToLower(wgt) directly
		 * on the buffer returned by Tcl_GetString(objv[i]),
		 * mutating that Tcl_Obj's string representation in place
		 * -- the same shared-object pitfall already fixed in
		 * BiotextCharsCmd (objv[] elements can be literal-pooled
		 * by Tcl's bytecode compiler across repeated calls).
		 * Lowercase a private copy instead. */
		char *wgtLower = (char *)ckalloc((strlen(wgt)+1)*sizeof(char));
		strcpy(wgtLower, wgt);
		Tcl_UtfToLower(wgtLower);
		if (strcmp(wgtLower, "normal") && strcmp(wgtLower, "bold")) {
		    /* Bug fix: Tcl_AddErrorInfo() only appends to the
		     * "errorInfo" traceback; it does not set the actual
		     * error message that Tcl_BackgroundError()/bgerror
		     * receives (that comes from the interpreter's result,
		     * via Tcl_SetObjResult). Since nothing set the result
		     * here, bgerror was invoked with an empty message,
		     * silently discarding "Font weight should be normal or
		     * bold" -- confirmed empty via a live bgerror capture
		     * in testing. Set the result explicitly so the actual
		     * message reaches the caller. */
		    Tcl_SetObjResult(interp, Tcl_NewStringObj(
			"Font weight should be normal or bold", -1));
		    Tcl_AddErrorInfo(interp, " : Font weight should be normal or bold");
		    Tcl_BackgroundError(interp);
		} else {
		    if (bioPtr->FontWeight != NULL) {
			ckfree((char *)bioPtr->FontWeight);
		    }
		    bioPtr->FontWeight = wgtLower;
		    wgtLower = NULL;
		}
		if (wgtLower != NULL) {
		    ckfree(wgtLower);
		}
	    }
	}
    }
  
 done:
    BiotextSetFont(bioPtr);
    // on est dans FontCmd
    BiotextComputeGeometry(bioPtr, -1, -1, -1);
  
    //ChangeBiotextXView(bioPtr, bioPtr->leftIndex);
    //ChangeBiotextYView(bioPtr, bioPtr->topIndex);
    bioPtr->flags |= UPDATE_SCROLLBAR;
    BiotextEventuallyRedraw(bioPtr);
  
    return TCL_OK;
}


/*--------------------------------------------------------------
 *
 * BiotextOutputCmd --
 *
 *      This procedure handles the "output" method.
 *      The sequences corresponding to the indices
 *      given are send as output result.
 *
 * Results:
 *	A Tcl object containing a list of sequences
 *
 * Side effects:
 *	None
 *
 *--------------------------------------------------------------
 */
static int 
BiotextOutputCmd(register Biotext *bioPtr,
		 Tcl_Interp *interp,
		 int objc,
		 Tcl_Obj * const objv[]) 
{
    int nseq, i, nelt, idx;
    Tcl_Size nseqSz;
    char *idxStr;
    Tcl_Obj **OIdx, *resObj, *idxObj, *Oelt;
  
    if (objc < 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "<all | index-list>");
	return TCL_ERROR;
    }
  
    if (bioPtr->nbSeqs < 1) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf("\nNo sequences in widget !"));
	return TCL_ERROR;
    }

    idxObj = Tcl_NewListObj(0, NULL);
    idxStr = Tcl_GetString(objv[2]);
    if (strcmp(idxStr, "all") == 0) {
	/* save all sequences */
	nelt = bioPtr->nbSeqs;
#ifdef DEBUG_OUTPUTCMD
	fprintf(stderr, "\n== in BiotextOutputCmd ==========\n");
	fprintf(stderr, "widget %s\n", bioPtr->widgetName);
	fprintf(stderr,"all sequences : %d\n", nelt);
	fflush(stderr);
#endif
	for (i=0; i<nelt; i++) {
	    Tcl_ListObjAppendElement(interp, idxObj, Tcl_NewIntObj(i));
	}
    } else {
	/* save sequences which indices are specified  in a list */
	if (Tcl_ListObjGetElements(interp,objv[2],&nseqSz,&OIdx) != TCL_OK) {
	    return TCL_ERROR;
	}
	nseq = (int)nseqSz;
	if (nseq == 0) {
	    return TCL_OK;
	}
#ifdef DEBUG_OUTPUTCMD
	fprintf(stderr, "\nnseq to output %d", nseq);
	fflush(stderr);
#endif
	nelt = nseq;
	for (i=0; i<nelt;i++) {
	    if (Tcl_GetIntFromObj(interp, OIdx[i], &idx) != TCL_OK) {
		return TCL_ERROR;
	    }
	    idx -= 1;
	    Tcl_ListObjAppendElement(interp, idxObj, Tcl_NewIntObj(idx));
	}
    }  
  
    resObj = Tcl_NewListObj(0, NULL);
#ifdef DEBUG_OUTPUTCMD  
    fprintf(stderr, "now outputing %d seqs\n", nelt);
    fflush(stderr);
#endif
    for(i=0; i<nelt; i++) {
	Tcl_ListObjIndex(interp, idxObj, i, &Oelt);
	Tcl_GetIntFromObj(interp, Oelt, &idx);
#ifdef DEBUG_OUTPUTCMD
	fprintf(stderr, "output seq idx %d i=%d\n", idx, i);
	fflush(stderr);
#endif
	/* Bug fix: idx comes straight from the user-supplied index list
	 * (1-based, converted to 0-based above) and was never
	 * bounds-checked before indexing bioPtr->SeqMat[idx] -- an
	 * out-of-range index (0, a typo, a stale index after rows were
	 * deleted, ...) read an arbitrary/garbage pointer and handed it
	 * to Tcl_NewStringObj() as a C string, an out-of-bounds read
	 * that could crash or leak adjacent heap memory into the result. */
	if (idx < 0 || idx >= bioPtr->nbSeqs) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf("bad sequence index \"%d\"", idx+1));
	    return TCL_ERROR;
	}
	Tcl_ListObjAppendElement(interp, resObj, Tcl_NewStringObj(bioPtr->SeqMat[idx], -1));
    }
  
    Tcl_SetObjResult(interp, resObj);

    return TCL_OK;
}


/*--------------------------------------------------------------
 *
 * BiotextCleanCmd --
 *
 *      This procedure handles the "clean" method.
 *      This commands desallocate all sequences, all 
 *      mapping, all tags definition and ranges. 
 *
 * Results:
 *	None
 *
 * Side effects:
 *	None
 *
 *--------------------------------------------------------------
 */
static void  
BiotextCleanCmd(register Biotext *bioPtr,
		Tcl_Interp *interp,
		int objc,
		Tcl_Obj * const objv[]) 
{
    int i, nbMap;

    /* Bug fix: bioPtr->nbMapping gets reset to 0 a few lines below
     * (as part of the SeqMat/MapSeq cleanup); capture its original
     * value now so the Mapping[] resource-cleanup loop further down
     * (added as part of this fix) actually has something to iterate
     * over instead of silently doing nothing. */
    nbMap = bioPtr->nbMapping;

    /* desallocate the sequences, mapping and tag matrix */
    if (bioPtr->SeqMat != NULL) {
	for (i=0; i<bioPtr->nbSeqs; i++) {
	    ckfree((char *)bioPtr->SeqMat[i]);
	    if (bioPtr->nbMapping > 0) {
		ckfree((char *)bioPtr->MapSeq[i]);
	    }
	}
	ckfree((char *)bioPtr->SeqMat);
	if (bioPtr->nbMapping >0) {
	    ckfree((char *)bioPtr->MapSeq);
	}
    }

    bioPtr->SeqMat = NULL;
    bioPtr->MapSeq = NULL;
    /* re-initialize counters */
    bioPtr->nbSeqs = 0;
    bioPtr->nbMapping = 0;
    bioPtr->lgSeqs = 0;

    /* desallocate groups */
    /* Bug fix: SeqGrp was freed but not reset to NULL, so the widget
     * destroy procedure freed it a second time ("alloc: invalid block"
     * abort when leaving Ordalie after File -> Close). */
    if(bioPtr->SeqGrp != NULL) {
	ckfree((char *)bioPtr->SeqGrp);
    }
    bioPtr->SeqGrp = NULL;
    bioPtr->nbrGrps = 0;

    /* desallocate tags */
    /* Bug fix: same leak pattern already fixed in
     * BiotextTagDeleteSubCmd/BiotextRebuildTagTable/BiotextMappingCmd
     * -- only tagName was freed here; Cfg/Cbg (color allocations),
     * bgBorder, gc and coords (a Tcl_Obj reference) leaked on every
     * ".biotext clean" call. */
    for (i=0; i<bioPtr->nbTags; i++) {
	ckfree((char *) bioPtr->Tags[i].tagName);
	if (bioPtr->Tags[i].Cfg != NULL) {
	    Tk_FreeColor(bioPtr->Tags[i].Cfg);
	}
	if (bioPtr->Tags[i].Cbg != NULL) {
	    Tk_FreeColor(bioPtr->Tags[i].Cbg);
	}
	if (bioPtr->Tags[i].bgBorder != NULL) {
	    Tk_Free3DBorder(bioPtr->Tags[i].bgBorder);
	}
	if (bioPtr->Tags[i].gc != NULL) {
	    Tk_FreeGC(bioPtr->display, bioPtr->Tags[i].gc);
	}
	if (bioPtr->Tags[i].coords != NULL) {
	    Tcl_DecrRefCount(bioPtr->Tags[i].coords);
	}
    }
    ckfree((char *)bioPtr->Tags);
    bioPtr->Tags = NULL;
    bioPtr->nbTags = 0;
    /* Bug fix (found via unit testing): nbTagsAlloc was never reset
     * here (this was already true in the original, unpatched code,
     * not something introduced by the earlier leak fix above). The
     * next "tag configure" for a brand-new tag checks
     * "if (bioPtr->nbTags >= bioPtr->nbTagsAlloc)" to decide whether
     * it needs to (re)allocate bioPtr->Tags; with nbTags reset to 0
     * but nbTagsAlloc left at its stale pre-clean value, that check
     * saw "already enough room" and skipped allocating a new Tags
     * array entirely -- writing straight into the just-freed/NULL
     * bioPtr->Tags pointer. Confirmed via gdb: SIGSEGV in
     * BiotextTagConfigSubCmd at "bioPtr->Tags[iTag].Cfg = Cfg;" on
     * the first tag configured after a ".biotext clean". */
    bioPtr->nbTagsAlloc = 0;
    /* Bug fix: the display order of the tags (listTags, indices into
     * Tags[]) was not emptied. After a "clean" (Ordalie : File ->
     * Reload, or Close then Open), it still started with the indices
     * of the old tags : "tag names" listed old names again ("Fnd Hel"
     * twice) and the drawing and "tag names" loops, bounded by nbTags,
     * never reached the new tags appended at the end (Ordalie
     * annotations Anno_* neither displayed nor hidden after a reload).
     * Same reset as in the "tag delete" of all the tags. */
    bioPtr->listTags = Tcl_NewListObj(0, NULL);

    /* desallocate mapping */
    /* Bug fix: same leak pattern as above, for the residue mapping
     * table (see also BiotextMappingCmd's own fix for the identical
     * issue on reconfiguration). */
    if (bioPtr->Mapping != NULL) {
	for (i=0; i<nbMap; i++) {
	    if (bioPtr->Mapping[i].Cfg != NULL) {
		Tk_FreeColor(bioPtr->Mapping[i].Cfg);
	    }
	    if (bioPtr->Mapping[i].Cbg != NULL) {
		Tk_FreeColor(bioPtr->Mapping[i].Cbg);
	    }
	    if (bioPtr->Mapping[i].bgBorder != NULL) {
		Tk_Free3DBorder(bioPtr->Mapping[i].bgBorder);
	    }
	    if (bioPtr->Mapping[i].gc != NULL) {
		Tk_FreeGC(bioPtr->display, bioPtr->Mapping[i].gc);
	    }
	}
	ckfree((char *)bioPtr->Mapping);
    }
    bioPtr->Mapping = NULL;

    /* desallocate cache buffers */
    BiotextInitBuffer(bioPtr, 0);
      
    return;
};


/*
 *--------------------------------------------------------------
 *
 * BiotextGetIndex --
 *	Parse an index position inside a biotext widget and return 
 *      either its value or an error.
 *
 * Results:
 *	A standard Tcl result.  If all went well, 
 *      then *row,*col is
 *	filled in with the index corresponding to 
 *      the <index> string.  If an
 *	error occurs then an error message is 
 *      left in interp result.
 *
 * Side effects:
 *      None.      
 *
 *--------------------------------------------------------------
 */
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

/*
 *--------------------------------------------------------------
 *
 * Biotext_Pixels2Coords --
 *      Gives the correspondance from pixels 
 *      coordinates to characters coordinates
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      None.      
 *
 *--------------------------------------------------------------
 */

static void
Biotext_Pixels2Coords(register Biotext *bioPtr,
		      int x,
		      int y,
		      int *c,
		      int *r) 
{

    *c = ((x - bioPtr->inset)/bioPtr->charWidth) + bioPtr->leftIndex;
    *r = ((y - bioPtr->inset)/bioPtr->charHeight) + bioPtr->topIndex;
    //rR fprintf(stderr, "pixels2coords :\ny= %d charH %d topI %d\nr= %d c= %d\n", y, bioPtr->charHeight, bioPtr->topIndex, *r, *c);
    //rR fflush(stderr);

    return;
}

/*
 *--------------------------------------------------------------
 *
 * Biotext_Coords2Pixels --
 *      Gives the correspondance from character 
 *      coordintates to pixels coordinates.
 *      The pixels coordinates are the coordinates of
 *      the upper-left corner of the character.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      None.      
 *
 *--------------------------------------------------------------
 */

static void
Biotext_Coords2Pixels(register Biotext *bioPtr,
		      int c,
		      int r,
		      int *x,
		      int *y,
		      int flag) 
{
    /*
     * flag = 0 : the output values are on a absolute 
     * scale.
     * flag = 1 : the output values are relative to the
     * border of the window
     */

    if (flag) {
	*x = (c - bioPtr->leftIndex)*bioPtr->charWidth + bioPtr->inset;
	*y = (r - bioPtr->topIndex )*bioPtr->charHeight + bioPtr->inset;
    } else {
	*x = c * bioPtr->charWidth + bioPtr->inset;
	*y = r * bioPtr->charHeight + bioPtr->inset;
    }

    return;
}

/*
 *--------------------------------------------------------------
 *
 * Biotext_IsVisible --
 *      Returns 1 if pixel/character at x,y
 *      is visible on the screen. Pixel/Char depends
 *      on the flag value.
 * 
 * Results:
 *      None.
 *
 * Side effects:
 *      None.      
 *
 *--------------------------------------------------------------
 */
static int 
Biotext_IsVisible(register Biotext *bioPtr, 
		  int xc, 
		  int yc, 
		  int flag) 
{
    int retVal;
  
    /* 
       flag = 1 -> character, 
       flag = 0 -> pixels
    */

    if (flag) {
	if ((xc >= bioPtr->leftIndex) && (yc >= bioPtr->topIndex) && (xc < bioPtr->leftIndex + bioPtr->widthChar) && (yc < bioPtr->topIndex + bioPtr->heightChar)) {
	    retVal = 1;
	} else {
	    retVal = 0;
	}
    } else {
	/* pixels */
	if ( (xc > 0 && yc > 0) && (xc < Tk_Width(bioPtr->tkwin)) && (yc < Tk_Height(bioPtr->tkwin))) {
	    retVal = 1;
	} else {
	    retVal = 0;
	}
    }
  
    return (retVal);
}


/*--------------------------------------------------------------
 *
 * BiotextDeleteGroup --
 *
 *      This routine deletes a group
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None
 *
 *--------------------------------------------------------------
 */
static int 
BiotextDeleteGroup(register Biotext *bioPtr,
		   int nm) 
{
    int i, val, found = 0;
    Tcl_Obj *el;

    for(i=0;i<bioPtr->nbrGrps;i++) {
	Tcl_ListObjIndex(bioPtr->interp, bioPtr->GroupList, i, &el);
	Tcl_GetIntFromObj(bioPtr->interp, el, &val);
	if (val == nm) {
	    Tcl_ListObjReplace(bioPtr->interp, bioPtr->GroupList, i, 1, 0, NULL);
	    found = 1;
	    break;
	}
    }
  
    /* Bug fix: nbrGrps was decremented unconditionally, even when "nm"
     * was not actually found/removed from GroupList -- that would
     * desync nbrGrps from GroupList's real length (nbrGrps under-
     * counting it), causing later loops bounded by nbrGrps to silently
     * miss the last real entries in GroupList. Currently unreachable
     * given the only call site (BiotextUngroupCmd always passes a
     * value it just read from GroupList itself), but cheap to make
     * robust against future callers. */
    if (found) {
	bioPtr->nbrGrps--;
    }
  
    return TCL_OK;
}


/*--------------------------------------------------------------
 *
 * BiotextFindNextGapOfGroup --
 *
 *      This routine searches the closest gap starting
 *      at index for sequences belonging to the group
 *      iGrp. Search to the left if
 *      index is negative, to the right otherwise.
 *
 * Results:
 *	column index of the closest gap
 *
 * Side effects:
 *	None
 *
 *--------------------------------------------------------------
 */
static int 
BiotextFindNextGapOfGroup(register Biotext *bioPtr,
			  int iseq,
			  int iGrp,
			  int icol,
			  int direction) 
{
    int i, j, res, cmp, lg, nall, start, stop, *tab;

    lg = bioPtr->lgSeqs;

    /* if direction = 1, search left, otherwise right */
    if (direction == 1) {
	start = 0;
	stop  = icol + 1;
	nall  = icol + 1;
    } else {
	start = icol + 1;
	stop  = lg;
	nall  = lg-icol;
    }

    tab = (int *)ckalloc(nall*sizeof(int));
    for (i=0;i<nall;i++) {
	tab[i]=0;
    }

    for(i=0;i<bioPtr->nbSeqs;i++) {
	if (bioPtr->SeqGrp[i] == iGrp) {
	    for (cmp=0,j=start;j<stop;j++,cmp++) {
		if (bioPtr->SeqMat[i][j] != '.')
		    tab[cmp]++;
	    }
	}
    }

    if (direction == 1) {
	res = -1;
	for (i=nall-1;i>=0;i--) {
	    if (tab[i] == 0) {
		res = i;
		break;
	    }
	}
    } else {
	res = nall + icol + 1;
	for (i=0;i<nall;i++) {
	    if (tab[i] == 0) {
		res = i+icol+1;
		break;
	    }
	}
    }

    ckfree((char *)tab);
  
    return res;
}


/*--------------------------------------------------------------
 *
 * BiotextFindNextGap --
 *
 *      This routine searches the closest gap starting
 *      at index for sequence i. Search to the left if
 *      index is negative, to the right otherwise.
 *
 * Results:
 *	column index of the closest gap
 *
 * Side effects:
 *	None
 *
 *--------------------------------------------------------------
 */
static int 
BiotextFindNextGap(register Biotext *bioPtr,
		   int iseq,
		   int icol,
		   int direction) 
{
    int pos;
  
    pos = icol;
    while (bioPtr->SeqMat[iseq][pos] != '.') {
	if (direction) {
	    /* search to the left */
	    pos--;
	    if (pos < 0) {
		break;
	    }
	} else {
	    /* search to the right */
	    pos++;
	    if (pos == bioPtr->lgSeqs) {
		break;
	    }
	}
    }
  
    return pos;
}


/*--------------------------------------------------------------
 *
 * BiotextPushInSeq --
 *
 *      This routine "pushes" sequences: it removes or
 *      insets a gap at imax , and removes/inserts a 
 *      gap at icol , depending if imax > icol or not.
 *
 * Results:
 *	column index of the closest gap
 *
 * Side effects:
 *	None
 *
 *--------------------------------------------------------------
 */
static void
BiotextPushInSeq(register Biotext *bioPtr,
		 int iseq,
		 int icol,
		 int imax) 
{
    Tcl_Interp *interp = bioPtr->interp;
    int i, iMap, left;

#if 0
        fprintf(stderr, "\nPushInSeq\niseq %d icol %d imax %d\n", iseq, icol, imax);
        fflush(stderr);
    #endif
    
    /* tag for insertion character */
    iMap = GetIndexOfTag(bioPtr, ".", 0);;

    /* push to the left */
    if (imax < icol) {
	left = 1;
	for(i=imax; i<icol; i++) {
	    bioPtr->SeqMat[iseq][i] = bioPtr->SeqMat[iseq][i+1];
	    if (bioPtr->nbMapping > 0) {
		bioPtr->MapSeq[iseq][i] = bioPtr->MapSeq[iseq][i+1];
	    }
	}
    } else {
	/* push to the right */
	left = 0;
	for(i=imax; i>icol; i--) {
	    bioPtr->SeqMat[iseq][i] = bioPtr->SeqMat[iseq][i-1];
	    if (bioPtr->nbMapping > 0) {
		bioPtr->MapSeq[iseq][i] = bioPtr->MapSeq[iseq][i-1];
 	    }
	}
    }
    bioPtr->SeqMat[iseq][icol] = '.';
    if (bioPtr->nbMapping > 0) {
	bioPtr->MapSeq[iseq][icol] = iMap;
    }
      
    /* push tags boundaries */
    if (bioPtr->nbTags > 0) {
	int j, k, nval, val, iTag, nelt;
	Tcl_Size neltSz, nvalSz;
	Tcl_Obj *tagObj, **eltObj, *coordsObj, **valObj, *tmpObj;
	//DumpCoordsTag(bioPtr, "entre push");
	for(i=0; i<bioPtr->nbTags; i++) {
	    Tcl_ListObjIndex(NULL, bioPtr->listTags, i, &tagObj);
	    Tcl_GetIntFromObj(NULL, tagObj, &iTag);
	    
	    /* Bug fix: this used to call Tcl_ListObjGetElements() directly
	     * on the LIVE bioPtr->Tags[iTag].coords (no duplication), then
	     * mutate one of its elements in place a few lines below
	     * ("eltObj[iseq] = Tcl_DuplicateObj(tmpObj);"). coords can be
	     * shared (e.g. with a pending copy/cut cache after "tag" data
	     * was copied along with rows), and Tcl requires such shared
	     * objects not be mutated through their internal
	     * representation. Duplicate first, like BiotextTagAddSubCmd/
	     * BiotextTagRemoveSubCmd already do, so only a privately-owned
	     * copy is touched here. */
	    coordsObj = Tcl_DuplicateObj(bioPtr->Tags[iTag].coords);
	    Tcl_ListObjGetElements(NULL, coordsObj, &neltSz, &eltObj);
	    nelt = (int)neltSz;
	    tmpObj = Tcl_NewListObj(0, NULL);
	    Tcl_ListObjGetElements(interp, eltObj[iseq], &nvalSz, &valObj);
	    nval = (int)nvalSz;
	    for(j=0; j<nval; j++) {
		Tcl_GetIntFromObj(NULL, valObj[j], &val);
		if(left) {
		    if(val >= imax && val <= icol) {
			val--;
		    }
		} else {
		    if(val >= icol && val <= imax) {
			val++;
		    }
		}
		Tcl_ListObjAppendElement(NULL, tmpObj, Tcl_NewIntObj(val));
	    }
	    eltObj[iseq] = Tcl_DuplicateObj(tmpObj);

	    {
		Tcl_Obj *newCoordsObj = Tcl_NewListObj(0, NULL);
		/* Bug fix: this used to do
		 *   Tcl_ListObjAppendElement(NULL, coordsObj2, Tcl_DuplicateObj(eltObj[k]));
		 *   Tcl_DecrRefCount(eltObj[k]);
		 * for every row k, including every row that was NOT
		 * touched above. For those, eltObj[k] used to be a
		 * borrowed pointer straight into the LIVE (and possibly
		 * shared, see above) bioPtr->Tags[iTag].coords -- freeing
		 * it here, before that field is even reassigned a few
		 * lines below, is a use-after-free / heap corruption risk
		 * if anything else (like a pending copy/cut cache) still
		 * holds a reference to that same row object. Since
		 * coordsObj is now our own private duplicate (see above),
		 * eltObj[k] here is already ours to read from freely; it
		 * just doesn't need an extra, unmatched release. */
		for (k=0; k<nelt; k++) {
		    Tcl_ListObjAppendElement(NULL, newCoordsObj, Tcl_DuplicateObj(eltObj[k]));
		}
		bioPtr->Tags[iTag].coords = Tcl_DuplicateObj(newCoordsObj);
		Tcl_DecrRefCount(newCoordsObj);
	    }
	}
	//DumpCoordsTag(bioPtr, "sort push");
    }

    return;
}
    
/*--------------------------------------------------------------
 *
 * BiotextComputeGeometry --
 *
 *      This routine adjusts and requests for the 
 *      biotext widget. Size should correspond to an 
 *      integer number of characters.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None
 *
 *--------------------------------------------------------------
 */
static void
BiotextComputeGeometry(register Biotext *bioPtr,
		       int fontChanged,
		       int newHeight,
		       int gridRequested) 
{
    int bdi, nC, nR, widthPxl, heightPxl;
    
    bdi = bioPtr->borderWidth + bioPtr->highlightWidth;

    if (fontChanged) {
	/* 
	 * get pixels geometry usin current values
	 */
	widthPxl  = bioPtr->widthChar * bioPtr->charWidth + 2 * bdi;
	heightPxl = bioPtr->heightChar * bioPtr->lineHeight + 2 * bdi;

	bioPtr->width = widthPxl;
	bioPtr->height = heightPxl;
    } else {
	widthPxl = bioPtr->width;
	heightPxl = bioPtr->height;
    }
  
    nC = bioPtr->widthChar;
    if (nC <= 0) {
	nC = (widthPxl - 2 * bdi) / bioPtr->charWidth;
	if (nC < 1) {
	    nC = 1;
	}
    }
  
    nR = bioPtr->heightChar;
    if (nR <= 0) {
	nR = (heightPxl - 2 * bdi) / bioPtr->lineHeight;
	if (nR < 1) {
	    nR = 1;
	}
    }    

    Tk_GeometryRequest(bioPtr->tkwin, widthPxl, heightPxl);
    Tk_SetInternalBorder(bioPtr->tkwin, bioPtr->inset);

    if (gridRequested) {
	if (bioPtr->setGrid) {
	    Tk_SetGrid(bioPtr->tkwin, nC, nR, bioPtr->charWidth, bioPtr->lineHeight);
	} else {
	    Tk_UnsetGrid(bioPtr->tkwin);
	}
    }    

    return;
}


/*--------------------------------------------------------------
 *
 * BiotextInitBuffer --
 *
 *      This routine initialises or resets the buffer
 *      used to copy/cut/paste the sequences (the cache).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None
 *
 *--------------------------------------------------------------
 */
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

/*--------------------------------------------------------------
 *
 * BiotextCopySeqsToBuffer --
 *
 *      This routine copies a sequence given its index
 *      to the copy/paste buffer. It copies all the
 *      attributes of the sequences.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None
 *
 *--------------------------------------------------------------
 */
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

/*--------------------------------------------------------------
 *
 * BiotextAddColsToBufferSeqs --
 *
 *      This routines adds cols to the sequences 
 *      contained in the paste/copy buffer.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None
 *
 *--------------------------------------------------------------
 */
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


void
BiotextRebuildTagTable(Biotext *bioPtr) {
    Tcl_Interp *interp = bioPtr->interp;
    int len, iTag, i;
    Tcl_Size lenSz;
    Tcl_Obj *tagObj, *nameObj;
    Tag *newTags;
    char *kept;

    Tcl_ListObjLength(NULL, bioPtr->listTags, &lenSz);
    len = (int)lenSz;
    nameObj = Tcl_NewListObj(0, NULL);
    newTags = (Tag *)ckalloc(len * sizeof(Tag));
    /* Bug fix: track which of the OLD bioPtr->Tags[] entries survive
     * into the new compacted array. Any old entry that is NOT carried
     * over (i.e. belongs to a tag that was just deleted) must have its
     * resources (tagName, Cfg/Cbg, bgBorder, gc, coords) released
     * before the old array is ckfree()'d below -- previously they were
     * silently dropped/leaked on every "tag delete tagName". */
    kept = (char *)ckalloc(bioPtr->nbTags * sizeof(char));
    for (i=0; i<bioPtr->nbTags; i++) {
	kept[i] = 0;
    }
    for(i=0; i<len; i++) {
	Tcl_ListObjIndex(NULL, bioPtr->listTags, i, &tagObj);
	Tcl_GetIntFromObj(NULL, tagObj, &iTag);
	Tcl_ListObjAppendElement(interp, nameObj, Tcl_NewStringObj(bioPtr->Tags[iTag].tagName, -1));
	newTags[i] = bioPtr->Tags[iTag];
	kept[iTag] = 1;
    }
    for (i=0; i<bioPtr->nbTags; i++) {
	if (kept[i]) {
	    continue;
	}
	if (bioPtr->Tags[i].tagName != NULL) {
	    ckfree(bioPtr->Tags[i].tagName);
	}
	if (bioPtr->Tags[i].Cfg != NULL) {
	    Tk_FreeColor(bioPtr->Tags[i].Cfg);
	}
	if (bioPtr->Tags[i].Cbg != NULL) {
	    Tk_FreeColor(bioPtr->Tags[i].Cbg);
	}
	if (bioPtr->Tags[i].bgBorder != NULL) {
	    Tk_Free3DBorder(bioPtr->Tags[i].bgBorder);
	}
	if (bioPtr->Tags[i].gc != NULL) {
	    Tk_FreeGC(bioPtr->display, bioPtr->Tags[i].gc);
	}
	if (bioPtr->Tags[i].coords != NULL) {
	    Tcl_DecrRefCount(bioPtr->Tags[i].coords);
	}
    }
    ckfree(kept);
    ckfree((char *)bioPtr->Tags);
    bioPtr->Tags = newTags;
    bioPtr->nbTags = len;
    bioPtr->nbTagsAlloc = len;

    bioPtr->listTags = Tcl_NewListObj(0, NULL);
    for(i=0; i<len; i++) {
	Tcl_ListObjIndex(interp, nameObj, i, &tagObj);
	iTag = GetIndexOfTag(bioPtr, Tcl_GetString(tagObj), 1);
	Tcl_ListObjAppendElement(interp, bioPtr->listTags, Tcl_NewIntObj(iTag));
    }
  
    return;
};


int
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


void
DumpSeqs(Biotext *bioPtr,
	 char *title)
{
    int i, slen;

    fprintf(stderr, "\n== DumpSeqs ======\n");
    fprintf(stderr, "widget %s\n", bioPtr->widgetName);
    fprintf(stderr,"%s\n", title);
    slen = strlen(bioPtr->SeqMat[0]);
    fprintf(stderr, "len %d\n", slen);
    for (i=0; i<bioPtr->nbSeqs; i++) {
	fprintf(stderr, "%3d >%s<\n", i, bioPtr->SeqMat[i]);
    }
    fprintf(stderr, "===========\n");
    fflush(stderr);

    return;
}


void 
DumpCoordsTag(Biotext *bioPtr,
	      char *titre) 
{
    int i, j, k, nelt, iTag, len, val;
    Tcl_Size neltSz, lenSz;
    Tcl_Obj **tmpObj, **eltObj, *tagObj;

    fprintf(stderr, "\n== DumpCoordsTag ================\n");
    fprintf(stderr, "widget %s\n", bioPtr->widgetName);
    fprintf(stderr, "%s\n", titre);
    fflush(stderr);

    for(k=0; k<bioPtr->nbTags; k++) {
	Tcl_ListObjIndex(NULL, bioPtr->listTags, k, &tagObj);
	Tcl_GetIntFromObj(NULL, tagObj, &iTag);
	fprintf(stderr, "%s pos= %d iTag= %d\n", bioPtr->Tags[iTag].tagName, k, iTag);
	fflush(stderr);
    
	Tcl_ListObjGetElements(NULL, bioPtr->Tags[iTag].coords, &neltSz, &eltObj);
	nelt = (int)neltSz;
	fprintf(stderr, "nbr seqs from tag %d\n", nelt);
	for(i=0; i<nelt; i++) {
	    Tcl_ListObjLength(NULL, eltObj[i], &lenSz);
	    len = (int)lenSz;
	    if (len == 0) {
		continue;
	    }
	    fprintf(stderr, "iseq= %d ", i);
	    Tcl_ListObjGetElements(NULL, eltObj[i], &lenSz, &tmpObj);
	    len = (int)lenSz;
	    for (j=0; j<len; j++) {
		Tcl_GetIntFromObj(NULL, tmpObj[j], &val);
		fprintf(stderr, " %d", val);
	    }
	    fprintf(stderr, "\n");
	    fflush(stderr);
	}
    }
  
    return;
}


/*
 *----------------------------------------------------------------------
 *
 * BiotextInsertGapsInTags --
 *
 *	This procedure is a helper procedure called to 
 * insert gaps in tags if needed.
 * Given a row and the index of the column where gap is inserted,
 * it checks the boundaries of all tags and evaluates if this boundaries 
 * should be changed or not.
 *
 * Results:
 *	The return value is a standard Tcl result. 
 *
 * Side effects:
 * The list of tags lists is updated.
 *
 *----------------------------------------------------------------------
 */
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

/*
 *----------------------------------------------------------------------
 *
 * ChangeBiotextYView --
 *
 *	Change the Y-view on a biotext widget so that a given element is
 *	displayed at the top.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	What's displayed on the screen is changed. If there is a scrollbar
 *	associated with this widget, then the scrollbar is instructed to
 *	change its display too.
 *
 *----------------------------------------------------------------------
 */
static void
ChangeBiotextYView(register Biotext *bioPtr,	/* Information about widget. */
		   int index)			/* Index of element in bioPtr that should now
						 * appear at the top of the biotext. */
{
#ifdef DEBUG_YVIEW
    fprintf(stderr, "\n== BiotextYViewCMD ========\n");
    fprintf(stderr, "widget %s\n", bioPtr->widgetName);
    fprintf(stderr, "index in : %d\n", index);
    fflush(stderr);
#endif
    if (index >= (bioPtr->nbSeqs - bioPtr->heightChar)) {
	index = bioPtr->nbSeqs - bioPtr->heightChar;
    }
    if (index < 0) {
	index = 0;
    }
#ifdef DEBUG_YVIEW
    fprintf(stderr, "topIndex %d index out %d\n", bioPtr->topIndex, index);
    fprintf(stderr, "-----\n\n");
    fflush(stderr);
#endif
    if (bioPtr->topIndex != index) {
	bioPtr->topIndex = index;
	bioPtr->flags |= UPDATE_SCROLLBAR;
	BiotextEventuallyRedraw(bioPtr);
    }
  
    return;
}


/*
 *----------------------------------------------------------------------
 *
 * ChangeBiotextXView --
 *
 *	Change the X-view on a biotext widget so that a given element is
 *	displayed at the left.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	What's displayed on the screen is changed. If there is a scrollbar
 *	associated with this widget, then the scrollbar is instructed to
 *	change its display too.
 *
 *----------------------------------------------------------------------
 */
static void
ChangeBiotextXView(register Biotext *bioPtr,	/* Information about widget. */
		   int index)			/* Index of element in bioPtr that should now
						 * appear at the left of the biotext. */
{
    if (index >= (bioPtr->lgSeqs - bioPtr->widthChar)) {
	index = bioPtr->lgSeqs - bioPtr->widthChar;
    }
    if (index < 0) {
	index = 0;
    }
    if (bioPtr->leftIndex != index) {
	bioPtr->leftIndex = index;

	BiotextEventuallyRedraw(bioPtr);
	bioPtr->flags |= UPDATE_SCROLLBAR;
    }

    return;
}


/*
 *--------------------------------------------------------------
 *
 * BiotextTagConfigSubCmd --
 *
 *	This procedure is invoked to process the 
 *      "tag config" sub-command. It configures a new tag or re-configure an existing tag.

 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None
 *
 *--------------------------------------------------------------
 */
static int 
BiotextTagConfigSubCmd(Biotext *bioPtr,
		       Tcl_Interp *interp,
		       int objc,
		       Tcl_Obj * const objv[]) 
{
    int i, j, iTag;
    int exists;
    char *tagName;
    char *colorNameFg, *colorNameBg;
    XColor *Cfg, *Cbg;
    Tk_3DBorder bgBorder;
    unsigned long fg, bg;
  
    if (objc != 8) {
	Tcl_WrongNumArgs(interp, 1, objv, "tag configure tagName -foreground fgColor -background bgColor");
	return TCL_ERROR;
    }

    if (bioPtr->nbSeqs <= 0) {
	return TCL_OK;
    }
  
    tagName = Tcl_GetString(objv[3]);
#ifdef DEBUG_TAGCONFIGCMD
    fprintf(stderr, "\n== in BiotextTagConfigSubCmd ==========\n");
    fprintf(stderr, "widget %s\n", bioPtr->widgetName);
    fprintf(stderr, ">%s<\n",tagName);
    fflush(stderr);
#endif
  
    /* Now take the tag attributes */
    bg = fg = -1;
    bgBorder = NULL;
    Cfg = Cbg = NULL;
  
    for (i=4;i<8;i+=2) {
	if (strcasecmp(Tcl_GetString(objv[i]),"-foreground") == 0 || strcasecmp(Tcl_GetString(objv[i]),"-fg") == 0) {
	    colorNameFg = Tcl_GetString(objv[i+1]);
#ifdef DEBUG_TAGCONFIGCMD
	    fprintf(stderr, "-foreground %s\n", colorNameFg);
	    fflush(stderr);
	    #endif
	    Cfg = Tk_GetColor(interp, bioPtr->tkwin, colorNameFg);
	    //Cfg = Tk_AllocColorFromObj(interp, bioPtr->tkwin, objv[i+1]);
	    /* Bug fix: Tk_GetColor() returns NULL on an invalid/unknown
	     * color name (e.g. a typo in the -foreground value); the
	     * code used to dereference it unconditionally right below
	     * ("fg = Cfg->pixel;"), crashing on any bad color name. */
	    if (Cfg == NULL) {
		return TCL_ERROR;
	    }
	    fg  = Cfg->pixel;
	}
	if (strcasecmp(Tcl_GetString(objv[i]),"-background") == 0 || strcasecmp(Tcl_GetString(objv[i]),"-bg") == 0) {
	    colorNameBg = Tcl_GetString(objv[i+1]);
#ifdef DEBUG_TAGCONFIGCMD
	    fprintf(stderr, "-background %s\n", colorNameBg);
	    fflush(stderr);
	    #endif
	    Cbg = Tk_GetColor(interp, bioPtr->tkwin, colorNameBg);
	    //Cbg = Tk_AllocColorFromObj(interp, bioPtr->tkwin, objv[i+1]);
	    /* Bug fix: same NULL-on-bad-color-name issue as -foreground
	     * above ("bg = Cbg->pixel;" would crash otherwise). */
	    if (Cbg == NULL) {
		if (Cfg != NULL) {
		    Tk_FreeColor(Cfg);
		}
		return TCL_ERROR;
	    }
	    bg  = Cbg->pixel;
	    //bgBorder = Tk_Get3DBorderFromObj(bioPtr->tkwin,objv[i+1]);
	    bgBorder = Tk_Get3DBorder(interp, bioPtr->tkwin, colorNameBg);
	}
    }
    /* Are colors OK ? */
    if (bg == -1 || fg == -1) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf("Error in background or foreground colors !"));
	return TCL_ERROR;
    }
  
    /* check if tag already exists */
    iTag = GetIndexOfTag(bioPtr, tagName, 1);
    if (iTag == -1) {
	/* NEW tag ! */
	exists = 0;
	iTag = bioPtr->nbTags;
	bioPtr->nbTags++;
      
	/* Is there place for a new tag ? */
	if (bioPtr->nbTags >= bioPtr->nbTagsAlloc) {
	    /* first pass: allocate */
	    if (bioPtr->nbTagsAlloc == 0) {
		bioPtr->nbTagsAlloc = 10;
		bioPtr->Tags = (Tag *)ckalloc(bioPtr->nbTagsAlloc*sizeof(Tag));
		for(i=0; i<bioPtr->nbTagsAlloc; i++) {
		    (bioPtr->Tags[i]).coords = Tcl_NewListObj(0, NULL);
		    for(j=0; j<bioPtr->nbSeqs; j++) {
			Tcl_ListObjAppendElement(interp, bioPtr->Tags[i].coords, Tcl_NewListObj(0, NULL));
		    }
		}
	    } else {
		/* not enough place, realloc */
		bioPtr->nbTagsAlloc += 10;
		bioPtr->Tags = (Tag *)ckrealloc((char *)bioPtr->Tags, bioPtr->nbTagsAlloc*sizeof(Tag));
		for(i=bioPtr->nbTagsAlloc-10; i<bioPtr->nbTagsAlloc; i++) {
		    bioPtr->Tags[i].coords = Tcl_NewListObj(0, NULL);
		    for(j=0; j<bioPtr->nbSeqs; j++) {
			Tcl_ListObjAppendElement(interp, bioPtr->Tags[i].coords, Tcl_NewListObj(0, NULL));
		    }
		}
	    }
	}
#ifdef DEBUG_TAGCONFIGCMD
	int len; Tcl_Size lenSz;
	Tcl_ListObjLength(interp, bioPtr->Tags[iTag].coords, &lenSz);
	len = (int)lenSz;
	fprintf(stderr, "tag length set: %d\n", len);
	fprintf(stderr,"------------------\n");
	fflush(stderr);
#endif
    } else {
	/* Tag already exists */
	exists = 1;
#ifdef DEBUG_TAGCONFIGCMD
	fprintf(stderr,"==>Tag exists !\n");
	fflush(stderr);
#endif
    }

#ifdef DEBUG_TAGCONFIGCMD
    fprintf(stderr,"nbTags %d <-> ",bioPtr->nbTags-1);
    fprintf(stderr, "%d =iTag\ntagName= %s\n", iTag, tagName);
    fflush(stderr);
#endif
  
    /* fill the tag structure */
    /* Bug fix: when reconfiguring an EXISTING tag (exists==1), the old
     * Cfg/Cbg (each an X server color-cell reference obtained via
     * Tk_GetColor) were overwritten here without ever being released
     * via Tk_FreeColor -- leaking one color allocation per axis every
     * time a tag's colors are changed (e.g. interactive color tweaking
     * in an editing session). bgBorder is a Tk_3DBorder, also
     * ref-counted by Tk and released the same way. tag->gc itself is
     * already correctly freed/recreated inside
     * Biotext_AssignColorOfTag(), so it needs no extra handling here. */
    if (exists) {
	if (bioPtr->Tags[iTag].Cfg != NULL) {
	    Tk_FreeColor(bioPtr->Tags[iTag].Cfg);
	}
	if (bioPtr->Tags[iTag].Cbg != NULL) {
	    Tk_FreeColor(bioPtr->Tags[iTag].Cbg);
	}
	if (bioPtr->Tags[iTag].bgBorder != NULL) {
	    Tk_Free3DBorder(bioPtr->Tags[iTag].bgBorder);
	}
    }
    bioPtr->Tags[iTag].Cfg = Cfg;
    bioPtr->Tags[iTag].Cbg = Cbg;
    bioPtr->Tags[iTag].fg  = fg;
    bioPtr->Tags[iTag].bg  = bg;
    bioPtr->Tags[iTag].bgBorder = bgBorder;
    /* Bug fix: this used to unconditionally set gc = NULL here, right
     * before calling Biotext_AssignColorOfTag(). For an EXISTING tag
     * being reconfigured, that discarded the live GC pointer *before*
     * Biotext_AssignColorOfTag() could see it and free it via its own
     * "if (tag->gc != NULL) Tk_FreeGC(...)" check -- leaking the old
     * GC on every recolor of an existing tag. For a brand-new tag
     * (ckalloc'd, not zero-initialized), gc still needs to be
     * explicitly NULLed so that same check doesn't try to Tk_FreeGC()
     * garbage memory -- so keep the assignment, just only for new tags. */
    if (! exists) {
	bioPtr->Tags[iTag].gc = NULL;
    }
    Biotext_AssignColorOfTag(&(bioPtr->Tags[iTag]), bioPtr->tkfont, bioPtr->tkwin, bioPtr->display);
  
    if (! exists) {
	bioPtr->Tags[iTag].smap = 0;
	bioPtr->Tags[iTag].tagName = (char *)ckalloc((strlen(tagName)+1)*sizeof(char));
	strcpy(bioPtr->Tags[iTag].tagName,tagName);
	Tcl_ListObjAppendElement(interp, bioPtr->listTags, Tcl_NewIntObj(iTag));
    }
    //DumpCoordsTag(bioPtr, "Fin TagConfig");
#ifdef DEBUG_TAGCONFIGCMD
    fprintf(stderr, "total nbTags  %d\n", bioPtr->nbTags);
    fflush(stderr);
#endif
  
    return TCL_OK;
}


/*
 *--------------------------------------------------------------
 *
 * BiotextTagAddSubCmd --
 *
 *	This procedure is invoked to process the 
 *      "tag add" sub-command. IIt adds a pre-configured tag to zones 
 * defined by a couple of indexes start end.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None
 *
 *--------------------------------------------------------------
 */
static int 
BiotextTagAddSubCmd(Biotext *bioPtr,
		    Tcl_Interp *interp,
		    int objc,
		    Tcl_Obj * const objv[]) 
{
    int i, iTag, row1, col1, row2, col2, tmp, nrow;
    Tcl_Size nrowSz;
    char *tagName, *index1, *index2;
    Tcl_Obj **rowObj, *coordsObj;
    Tcl_Obj *oldCoords, *newObj, **newRows;
    int code = TCL_OK;

    if (bioPtr->nbSeqs == 0) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf("The widget does not contain any sequences !"));
	return TCL_ERROR;
    }
    
    tagName = Tcl_GetString(objv[3]);
    /* the tag must have been previously defined */
    iTag = GetIndexOfTag(bioPtr, tagName, 1);
    if (iTag == -1) {
	/* unknown tag */
	Tcl_SetObjResult(interp, Tcl_ObjPrintf("Tag named %s does not exists !", tagName));
	return TCL_ERROR;
    }
    
    /* now check the indexes - start1 end1 start2 end2 .... startn endn
     * There must be an even number of indexes
     */
    if (((objc-4) % 2) != 0) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf("There must be an even number of indexes specifying zones (start1 end1 start2 end2 ... startn endn)."));
	return TCL_ERROR;
    }
    
    //DumpCoordsTag(bioPtr, "entre TagAdd");
    /* read the indexes */
    /* Bug fix: each index pair duplicated the whole row list, and neither
     * these copies nor the previous coords were ever released -- about
     * 15 MB lost per "tag add" on a conservation feature (Ordalie audit,
     * bug B4), with a cost quadratic in the number of zones of a row.
     * A row is now duplicated once, on its first change. Rows left
     * unchanged are shared with the old coords just long enough for
     * these to be released, so every row ends with a single owner. */
    oldCoords = bioPtr->Tags[iTag].coords;
    coordsObj = Tcl_DuplicateObj(oldCoords);
    Tcl_IncrRefCount(coordsObj);
    Tcl_ListObjGetElements(interp, coordsObj, &nrowSz, &rowObj);
    nrow = (int)nrowSz;
    if (nrow== 0) {
	nrow = bioPtr->nbSeqs;
	for (i=0; i<nrow; i++) {
	    Tcl_ListObjAppendElement(interp, coordsObj, Tcl_NewListObj(0, NULL));
	}
	Tcl_ListObjGetElements(interp, coordsObj, &nrowSz, &rowObj);
	nrow = (int)nrowSz;
    }
    newRows = (Tcl_Obj **)ckalloc(nrow*sizeof(Tcl_Obj *));
    memset(newRows, 0, nrow*sizeof(Tcl_Obj *));
    for (i=4; i<objc; i+=2) {
	/* row numbering starts at 1 */
	/* But BiotextGetIndex returns row and cols starting at 0 */
	index1 = Tcl_GetString(objv[i]);
	/* Bug fix: BiotextGetIndex()'s return value was never checked.
	 * On a malformed index (bad user input, e.g. a typo in a Tcl
	 * script), it returns TCL_ERROR *without* ever writing to
	 * row1/col1 -- leaving them uninitialized (garbage stack
	 * values). The code used to plow ahead and use that garbage as
	 * a row index into rowObj[], an out-of-bounds access, while
	 * also silently discarding the "bad index" error message that
	 * BiotextGetIndex had already set. */
	if (BiotextGetIndex(bioPtr, index1, &row1, &col1) != TCL_OK) {
	    code = TCL_ERROR;
	    goto done;
	}
	index2 = Tcl_GetString(objv[i+1]);
	if (BiotextGetIndex(bioPtr, index2, &row2, &col2) != TCL_OK) {
	    code = TCL_ERROR;
	    goto done;
	}

	if (row1 != row2) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf("\nError ! row1 should be equal to row2 !"));
	    code = TCL_ERROR;
	    goto done;
	}
      
	tmp = col1;
	col1 = MIN(col1,col2);
	col2 = MAX(tmp,col2);
      
#ifdef DEBUG_TAGADDCMD
	fprintf(stderr, "\n== in BiotextTagAddSubCmd ==========\n");
	fprintf(stderr, "widget %s\n", bioPtr->widgetName);
	fprintf(stderr, "tagName %s iTag %d\n", tagName, iTag);
	fprintf(stderr,"index1 %s index2 %s\n   r1= %d c1= %d\n   r2= %d c2= %d\n",index1, index2,row1, col1, row2, col2);
	fflush(stderr);
#endif
      
	if (newRows[row1] == NULL) {
	    newRows[row1] = Tcl_DuplicateObj(rowObj[row1]);
	    Tcl_IncrRefCount(newRows[row1]);
	}
	Tcl_ListObjAppendElement(interp, newRows[row1], Tcl_NewIntObj(col1));
	Tcl_ListObjAppendElement(interp, newRows[row1], Tcl_NewIntObj(col2));
    }
    newObj = Tcl_NewListObj(0, NULL);
    for (i=0; i<nrow; i++) {
	Tcl_ListObjAppendElement(interp, newObj, (newRows[i] != NULL) ? newRows[i] : rowObj[i]);
    }
    bioPtr->Tags[iTag].coords = newObj;
    Tcl_DecrRefCount(oldCoords);

    //DumpCoordsTag(bioPtr, "fort TagAdd");

 done:
    for (i=0; i<nrow; i++) {
	if (newRows[i] != NULL) {
	    Tcl_DecrRefCount(newRows[i]);
	}
    }
    ckfree((char *)newRows);
    Tcl_DecrRefCount(coordsObj);

    return code;
}


/*
 *--------------------------------------------------------------
 *
 * BiotextTagRemoveSubCmd --
 *
 *	This procedure is invoked to process the 
 *      "tag remove" sub-command. 
 *       It removes the named tag from the screen but keeps its definition.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None
 *
 *--------------------------------------------------------------
 */
static int 
BiotextTagRemoveSubCmd(Biotext *bioPtr,
		       Tcl_Interp *interp,
		       int objc,
		       Tcl_Obj * const objv[]) 
{
    int start, end, iTag, i, j, tmp, col1, row1, col2, row2;
    char *tagName, *index1, *index2;
  
    if (objc == 3) {
	/* remove all tags instances everywhere */
	start = 0;
	end = bioPtr->nbTags;
    } else if (objc ==4) {
	/* remove a given tag instances everywhere */
	tagName = Tcl_GetString(objv[3]);
	iTag = GetIndexOfTag(bioPtr, tagName, 1);
	if (iTag == -1) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf("tag named %s does not exists !", tagName));
	    return TCL_ERROR;
	}
	start = iTag;
	end = iTag+1;
    }

    if (objc < 5) {
	for (i=start; i<end; i++) {
	    bioPtr->Tags[i].coords = Tcl_NewListObj(0, NULL);
	    for(j=0; j<bioPtr->nbSeqs; j++) {
		Tcl_ListObjAppendElement(interp, bioPtr->Tags[i].coords, Tcl_NewListObj(0, NULL));
	    }
	}
	//DumpCoordsTag(bioPtr, "REMOVE TAG");
	goto done;
    }

    /* remove a given tag from region */
    /* There should be a even number of zones */
    if (((objc -4) % 2) > 0) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf("Error ! Zones to remove should be of the type start1 - end1 start2 - end2 ..."));
	return TCL_ERROR;
    }

    int nelt, len, lenRow, val1, val2;
    Tcl_Size neltSz, lenSz, lenRowSz;
    Tcl_Obj **eltObj, **rowObj, *newObj, *coordsObj;

    tagName = Tcl_GetString(objv[3]);
    iTag = GetIndexOfTag(bioPtr, tagName, 1);
    if (iTag == -1) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf("tag named %s does not exists !", tagName));
	return TCL_ERROR;
    }

    /* Bug fix: unlike BiotextTagAddSubCmd (which duplicates
     * bioPtr->Tags[iTag].coords before touching it), this used to call
     * Tcl_ListObjGetElements() directly on the LIVE coords object and
     * then mutate its elements in place ("eltObj[row1] = ...") a few
     * lines below. bioPtr->Tags[iTag].coords can be shared (e.g. with
     * a pending copy/cut cache), and Tcl's contract for shared objects
     * is that their internal representation must not be mutated in
     * place. Duplicate first, like BiotextTagAddSubCmd does, so the
     * loop below only ever touches a privately-owned copy. */
    coordsObj = Tcl_DuplicateObj(bioPtr->Tags[iTag].coords);
    Tcl_ListObjGetElements(NULL, coordsObj, &neltSz, &eltObj);
    nelt = (int)neltSz;
  
#ifdef DEBUG_TAGREMOVE
    fprintf(stderr, "\n== BiotextTagRemoveSubCmd =======\n");
    fprintf(stderr, "widget %s\n", bioPtr->widgetName);
    fprintf(stderr, "tagName %s iTag %d\n", tagName, iTag);
    fprintf(stderr, "nb elt %d\n", nelt);
    fflush(stderr);
#endif
    for(i=4; i<objc; i+=2) {
	index1 = Tcl_GetString(objv[i]);
	/* Bug fix: same missing return-value check as in
	 * BiotextTagAddSubCmd -- row1/col1 (and row2/col2) could be
	 * left uninitialized on a malformed index, then used as
	 * an out-of-bounds row index below. */
	if (BiotextGetIndex(bioPtr, index1, &row1, &col1) != TCL_OK) {
	    return TCL_ERROR;
	}
	index2 = Tcl_GetString(objv[i+1]);
	if (BiotextGetIndex(bioPtr, index2, &row2, &col2) != TCL_OK) {
	    return TCL_ERROR;
	}

#ifdef DEBUG_TAGREMOVE
	fprintf(stderr, "index1 %s row1 %d col1 %d\n", index1, row1, col1);
	fprintf(stderr, "index2 %s row2 %d col2 %d\n", index2, row2, col2);
	fflush(stderr);
#endif

	if (row1 != row2) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf("\nrow1 != row2 for tag remove !"));
	    return TCL_ERROR;
	}
      
	tmp = col1;
	col1 = MIN(col1,col2);
	col2 = MAX(tmp,col2);
      
	Tcl_ListObjLength(interp, eltObj[row1], &lenSz);
	len = (int)lenSz;
	if (len == 0) {
	    /* there is no tagged zones in row */
	    continue;
	}

#ifdef DEBUG_TAGREMOVE
	fprintf(stderr, "row1 %d col1 %d col2 %d\n", row1, col1, col2);
	fprintf(stderr, "nb zones %d\n", len);
	fflush(stderr);
#endif
	Tcl_ListObjGetElements(interp, eltObj[row1], &lenRowSz, &rowObj);
	lenRow = (int)lenRowSz;
	newObj = Tcl_NewListObj(0, NULL);
	for(j=0; j<lenRow; j+=2) {
	    Tcl_GetIntFromObj(interp, rowObj[j], &val1);
	    Tcl_GetIntFromObj(interp, rowObj[j+1], &val2);

#ifdef DEBUG_TAGREMOVE
	    fprintf(stderr, "val1 %d val2 %d\n", val1, val2);
	    fflush(stderr);
#endif
	    if (! (col1 == val1 && col2 == val2)) {
		Tcl_ListObjAppendElement(interp, newObj, Tcl_DuplicateObj(rowObj[j]));
		Tcl_ListObjAppendElement(interp, newObj, Tcl_DuplicateObj(rowObj[j+1]));
	    }
	}
	eltObj[row1] = Tcl_DuplicateObj(newObj);
    }
    newObj = Tcl_NewListObj(0, NULL);
    for (i=0; i< nelt; i++) {
	Tcl_ListObjAppendElement(interp, newObj, Tcl_DuplicateObj(eltObj[i]));
    }
    bioPtr->Tags[iTag].coords = Tcl_DuplicateObj(newObj);
 			  
 done:
  
    //DumpCoordsTag(bioPtr, "fin TagRemove");

    return TCL_OK;
}


/*
 *--------------------------------------------------------------
 *
 * BiotextTagDeleteSubCmd --
 *
 *	This procedure is invoked to process the 
 *      "tag delete" sub-command.
 *     It deletes both tag definition and cleans the tagged region on the screen.
 *If no tag name is supllied, then all tags are deleted  
 
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None
 *
 *--------------------------------------------------------------
 */
static int 
BiotextTagDeleteSubCmd(Biotext *bioPtr,
		       Tcl_Interp *interp,
		       int objc,
		       Tcl_Obj * const objv[]) 
{
    int iTag, i, j;
    char *tagName;
  
#ifdef DEBUG_TAGDELETECMD
    fprintf(stderr, "\n== in BiotextTagDeleteSubCmd ==========\n");
    fprintf(stderr, "widget %s\n", bioPtr->widgetName);
    fflush(stderr);
#endif

    if (objc == 3) {
	/* delete ALL tags */
	/* Bug fix: this used to ckfree(bioPtr->Tags) directly, without
	 * releasing any of the per-tag resources first -- leaking every
	 * tag's tagName string, Cfg/Cbg color allocations, bgBorder, gc
	 * and coords Tcl_Obj on "tag delete" (no tag name given). */
	for (i=0; i<bioPtr->nbTags; i++) {
	    if (bioPtr->Tags[i].tagName != NULL) {
		ckfree(bioPtr->Tags[i].tagName);
	    }
	    if (bioPtr->Tags[i].Cfg != NULL) {
		Tk_FreeColor(bioPtr->Tags[i].Cfg);
	    }
	    if (bioPtr->Tags[i].Cbg != NULL) {
		Tk_FreeColor(bioPtr->Tags[i].Cbg);
	    }
	    if (bioPtr->Tags[i].bgBorder != NULL) {
		Tk_Free3DBorder(bioPtr->Tags[i].bgBorder);
	    }
	    if (bioPtr->Tags[i].gc != NULL) {
		Tk_FreeGC(bioPtr->display, bioPtr->Tags[i].gc);
	    }
	    if (bioPtr->Tags[i].coords != NULL) {
		Tcl_DecrRefCount(bioPtr->Tags[i].coords);
	    }
	}
	ckfree((char *)bioPtr->Tags);
      
	bioPtr->nbTags = 0;
	bioPtr->nbTagsAlloc = 0;
	bioPtr->listTags = Tcl_NewListObj(0, NULL);
    } else {
	/* delete specific tags */
	Tcl_Obj *tagObj;
	int val, iObj;
      
	for(i=3; i<objc; i++) {
	    tagName = Tcl_GetString(objv[i]);
#ifdef DEBUG_TAGDELETECMD
	    fprintf(stderr,"\ndelete tag >%s<\n",tagName);
	    fflush(stderr);
#endif
	    iTag = GetIndexOfTag(bioPtr, tagName, 1);
	    if (iTag == -1) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf("tag named %s does not exists !", tagName));
		return TCL_ERROR;
	    }
	    iObj = -1;
	    for(j=0; j<bioPtr->nbTags; j++) {
		Tcl_ListObjIndex(interp, bioPtr->listTags, j, &tagObj);
		Tcl_GetIntFromObj(NULL, tagObj, &val);
		if (val == iTag) {
		    iObj = j;
		    break;
		}
	    }
	    Tcl_ListObjReplace(interp, bioPtr->listTags, iObj, 1, 0, NULL);
	}
	BiotextRebuildTagTable(bioPtr);
    }
  
    return TCL_OK;
}


/*
 *--------------------------------------------------------------
 *
 * BiotextTagNamesSubCmd --
 *
 *	This procedure is invoked to process the 
 *      "tag names" sub-command.
 *It returns all tag names present in the widget.
 *If an index is supplied, it returns the tag names present at index.

 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None
 *
 *--------------------------------------------------------------
 */
static int 
BiotextTagNamesSubCmd(Biotext *bioPtr,
		      Tcl_Interp *interp,
		      int objc,
		      Tcl_Obj * const objv[]) 
{
    int iTag, i, j, k;
    Tcl_Obj *res, *tagObj;

    res = Tcl_NewListObj(0, NULL);
    if (objc == 4) {
	int r, c, r1, r2, c1, c2;
	Tcl_Size lenCoords;
	char *index;
	Tcl_Obj *cseqObj, *valObj;
      
	index = Tcl_GetString(objv[3]);
	if (BiotextGetIndex(bioPtr, index, &r, &c) != TCL_OK) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf("\nindex is not valid !"));
	    return TCL_ERROR;
	}
	/* Bug fix: BiotextGetIndex() already returns a 0-based row (it
	 * does its own "row -= 1" internally to convert from the
	 * public 1-based index syntax). Subtracting 1 again here
	 * double-decremented r (e.g. index "1.0" -> r=0 from
	 * BiotextGetIndex -> r=-1 here), which made the "r >= r1 && r
	 * <= r2" match below impossible for any real row -- confirmed
	 * live: "tag names <any index>" always returned an empty list,
	 * even for a position clearly inside a tagged zone (verified
	 * against "tag ranges", which agreed the zone existed). */
	for(k=0; k<bioPtr->nbTags; k++) {
	    Tcl_ListObjIndex(interp, bioPtr->listTags, k, &tagObj);
	    Tcl_GetIntFromObj(interp, tagObj, &iTag);
	    for(i=0; i<bioPtr->nbSeqs; i++) {
		r1 = r2 = i;
		Tcl_ListObjIndex(interp, bioPtr->Tags[iTag].coords, i, &cseqObj);
		Tcl_ListObjLength(interp, cseqObj, &lenCoords);
		for(j=0; j<lenCoords; j+=2) {
		    Tcl_ListObjIndex(interp, cseqObj, j, &valObj);
		    Tcl_GetIntFromObj(interp, valObj, &c1);
		    Tcl_ListObjIndex(interp, cseqObj, j+1, &valObj);
		    Tcl_GetIntFromObj(interp, valObj, &c2);
		    if (r >= r1 && r <= r2 && c >= c1 && c <= c2) {
			/* Bug fix: this used bioPtr->Tags[k].tagName, but
			 * "k" is the position within bioPtr->listTags (the
			 * list of *active* tag indices), not a Tag[] array
			 * index -- "iTag" (looked up from listTags[k] just
			 * above, and correctly used for ".coords" two lines
			 * up) is the right index into bioPtr->Tags[]. "k"
			 * and "iTag" only coincide as long as no tag has
			 * ever been deleted (listTags still the identity
			 * mapping); after any "tag delete", this returned
			 * the wrong tag name for "tag names <index>". */
			Tcl_ListObjAppendElement(interp, res, Tcl_NewStringObj(bioPtr->Tags[iTag].tagName, -1));
		    }
		}
	    }
	}
    } else {
	/* return ALL tags names */
	for(i=0; i<bioPtr->nbTags; i++) {
	    Tcl_ListObjIndex(interp, bioPtr->listTags, i, &tagObj);
	    Tcl_GetIntFromObj(interp, tagObj, &iTag);
	    Tcl_ListObjAppendElement(interp, res, Tcl_NewStringObj(bioPtr->Tags[iTag].tagName,-1));
	    //fprintf(stderr,"tag name %s with i= %d\n", bioPtr->Tags[i].tagName, i);
	    //fflush(stderr);
	}
    }
    //DumpCoordsTag(bioPtr, "End tag names");
    Tcl_SetObjResult(interp, res);

    return TCL_OK;
}


/*
 *--------------------------------------------------------------
 *
 * BiotextTagStateSubCmd --
 *
 *	This procedure is invoked to process the 
 *      "tag state" sub-command.
 *Switch the tagging of sequences ON or OFF.

 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None
 *
 *--------------------------------------------------------------
 */
static int 
BiotextTagStateSubCmd(Biotext *bioPtr,
		      Tcl_Interp *interp,
		      int objc,
		      Tcl_Obj * const objv[]) 
{
    int zebool;
  
    if (objc != 4) {
	Tcl_WrongNumArgs(interp, 1, objv, "tag state boolean");
	return TCL_ERROR;
    }
    
    if (Tcl_GetBooleanFromObj(interp,objv[3],&zebool) != TCL_OK) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf("Error ! You should supply a boolean to tag state command !"));
	return TCL_ERROR;
    }
    
    if (zebool) {
	if (! (bioPtr->flags & TAG_ALIGNMENT)) {
	    bioPtr->flags |= TAG_ALIGNMENT;
	}
    } else {
	if (bioPtr->flags & TAG_ALIGNMENT) {
	    bioPtr->flags &= ~TAG_ALIGNMENT;
	}
    }

    return TCL_OK;
}


/*
 *--------------------------------------------------------------
 *
 * BiotextTagRaiseSubCmd --
 *
 *	This procedure is invoked to process the 
 *      "tag raise" sub-command.
 *This method pushes the designed tag name on top of the list of tags,
 *in such a way it will be on top of all displayed tags.
 
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None
 *
 *--------------------------------------------------------------
 */
static int 
BiotextTagRaiseSubCmd(Biotext *bioPtr,
		      Tcl_Interp *interp,
		      int objc,
		      Tcl_Obj * const objv[]) 
{
    int i, iTag, iObj, index, len;
    Tcl_Size lenSz;
    char *tagName;
    Tcl_Obj *tmpObj, *tagObj;
  
    if (objc != 4) {
	Tcl_WrongNumArgs(interp, 1, objv, "tag raise tagName");
	return TCL_ERROR;
    }

    if (bioPtr->listTags == NULL) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf("No tags defined !"));
	return TCL_ERROR;
    }
    
    if (Tcl_ListObjLength(interp, bioPtr->listTags, &lenSz) != TCL_OK) {
	return TCL_ERROR;
    }
    len = (int)lenSz;
    if (len == 0) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf("No tags defined !"));
	return TCL_ERROR;
    }
    
    tagName = Tcl_GetString(objv[3]);
    iTag = GetIndexOfTag(bioPtr, tagName, 1);
    if (iTag == -1) {
	/* tag does not exists */
	Tcl_SetObjResult(interp, Tcl_ObjPrintf("\nError ! Tag named %s does not exists while raising tag up !", tagName));
	return TCL_ERROR;
    }
    
    /* locate tag in the tag list */
    index = -1;
    for(i=0; i<len; i++) {
	Tcl_ListObjIndex(interp, bioPtr->listTags, i, &tagObj);
	Tcl_GetIntFromObj(NULL, tagObj, &iObj);
	if (iTag == iObj) {
	    index = i;
	    break;
	}
    }
    if (index == -1) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf("Tag named %s does not exists !", tagName));
	return TCL_ERROR;
    }

    tmpObj = Tcl_DuplicateObj(bioPtr->listTags);
    Tcl_ListObjReplace(interp, tmpObj, index, 1, 0, NULL);
    Tcl_ListObjAppendElement(interp, tmpObj, Tcl_DuplicateObj(tagObj));
    bioPtr->listTags = Tcl_DuplicateObj(tmpObj);

    return TCL_OK;
}


/*
 *--------------------------------------------------------------
 *
 * BiotextTagRangesSubCmd --
 *
 *	This procedure is invoked to process the 
 *      "tag ranges" sub-command. 
 *It returns a list of coordinates in the form row.col where the designed tag appears.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None
 *
 *--------------------------------------------------------------
 */
static int 
BiotextTagRangesSubCmd(Biotext *bioPtr,
		       Tcl_Interp *interp,
		       int objc,
		       Tcl_Obj * const objv[]) 
{
    int i, j, len, iTag, nrow, col;
    Tcl_Size lenSz, nrowSz;
    char *tagName, row[32];
    Tcl_Obj *tmpObj, *resObj, **rowObj;
  
    if (objc != 4) {
	Tcl_WrongNumArgs(interp, 1, objv, "tag ranges tagName");
	return TCL_ERROR;
    }

    if (bioPtr->listTags == NULL) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf("No tags defined !"));
	return TCL_ERROR;
    }
  
    if (Tcl_ListObjLength(interp, bioPtr->listTags, &lenSz) != TCL_OK) {
	return TCL_ERROR;
    }
    len = (int)lenSz;
    if (len == 0) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf("No tags defined !"));
	return TCL_ERROR;
    }
  
    tagName = Tcl_GetString(objv[3]);
    iTag = GetIndexOfTag(bioPtr, tagName, 1);
    if (iTag == -1) {
	/* tag does not exists */
	Tcl_SetObjResult(interp, Tcl_ObjPrintf("Tag named %s does not exists !", tagName));
	return TCL_ERROR;
    }
  
    resObj = Tcl_NewListObj(0, NULL);
    Tcl_ListObjGetElements(interp, bioPtr->Tags[iTag].coords, &nrowSz, &rowObj);
    nrow = (int)nrowSz;
    /* Bug fix: this loop used to bound on bioPtr->nbSeqs, while rowObj[]
     * actually only has "nrow" elements (the real length of the coords
     * list just queried above). These are expected to always match by
     * design, but relying on that invariant here instead of the length
     * we already have in hand risks an out-of-bounds read on rowObj[]
     * (and a likely crash from calling Tcl_ListObjLength/Index on a
     * garbage pointer) if it is ever violated -- e.g. by a tag whose
     * coords list wasn't kept in sync after a row was added. Loop on
     * "nrow" instead, which is guaranteed to match rowObj[]'s actual
     * size. */
    for (i=0; i < nrow; i++) {
	if (Tcl_ListObjLength(interp, rowObj[i], &lenSz) != TCL_OK) {
	    continue;
	}
	len = (int)lenSz;
	if (len <= 0) {
	    continue;
	}
      
	for (j=0; j < len; j++) {
	    Tcl_ListObjIndex(NULL, rowObj[i], j, &tmpObj);
	    Tcl_GetIntFromObj(NULL, tmpObj, &col);
	    sprintf(row, "%d.%d", i+1, col);
	    Tcl_ListObjAppendElement(NULL, resObj, Tcl_NewStringObj(row, -1));
	    row[0] = '\0';
	}
    }
    Tcl_SetObjResult(interp, resObj);
  
    return TCL_OK;
}

void
DumpNullChar(Biotext *bioPtr)
{
    int i, j, ok;

    fprintf(stderr,"\nfind nulls: \n");
    for (i=0; i<bioPtr->nbSeqs; i++) {
	ok = 0;
	for (j=bioPtr->lgSeqs-3; j<=bioPtr->lgSeqs; j++) {
	    if (bioPtr->SeqMat[i][j] == '\0') {
		ok = j;
		break;
	    }
	}
	fprintf(stderr,"seq %d null at %d\n", i, ok);
    }
    fflush(stderr);

    return;
}
    
void
Biotext_AssignColorOfTag(Tag *tag,
			 Tk_Font tkfont,
			 Tk_Window tkwin,
			 Display *display) 
{
    XGCValues gcValues;
    unsigned long mask;
  
#ifdef USE_XFT
    XColor *Cfg, *Cbg;

    Cfg = tag->Cfg;
    Cbg = tag->Cbg;
  
    tag->XftCfg.pixel       = Cfg[0].pixel;
    tag->XftCfg.color.red   = Cfg[0].red;
    tag->XftCfg.color.green = Cfg[0].green;
    tag->XftCfg.color.blue  = Cfg[0].blue;
    tag->XftCfg.color.alpha = 0xffff;
  
    tag->XftCbg.pixel       = Cbg[0].pixel;
    tag->XftCbg.color.red   = Cbg[0].red;
    tag->XftCbg.color.green = Cbg[0].green;
    tag->XftCbg.color.blue  = Cbg[0].blue;
    tag->XftCbg.color.alpha = 0xffff;
#endif
  
    /* Create a graphical context */
    if (tag->gc != NULL) {
	Tk_FreeGC(display, tag->gc);
    }
    tag->gc = NULL;
  
    gcValues.graphics_exposures = False;
    gcValues.font               = Tk_FontId(tkfont);
    gcValues.foreground         = tag->fg;
    //gcValues.background = Tk_3DBorderColor(bioPtr->fgBorder)->pixel;
    //gcValues.background         = tag->bg;

    mask = GCForeground | GCFont | GCGraphicsExposures;
  
    tag->gc = Tk_GetGC(tkwin,mask,&gcValues);
  
    return;
}


static void 
DumpMapping(Biotext *bioPtr,
	  char *title)
{
    int i, j;

    if (0 && ! (bioPtr->flags & MAP_RESIDUES)) {
	return;
    }

    fprintf(stderr,"\n%s\n", title);
    for (i=0;i< bioPtr->nbSeqs; i++) {
	fprintf(stderr,"seq %2d : ",i);
	for (j=0; j<bioPtr->lgSeqs; j++) {
	    fprintf(stderr, "%2d|", bioPtr->MapSeq[i][j]);
	}
	fprintf(stderr,"|\n");
    }
    fflush(stderr);

    return;
}

#ifdef DUMPCACHE
static void 
DumpCache(Biotext *bioPtr,
	  char *title)
{
    Tcl_Interp *interp = bioPtr->interp;
    int i, j, k, iTag, col;
    Tcl_Size len;
    Tcl_Obj *tagObj, *debObj, *rowObj;
    
    fprintf(stderr, "\n== DumpCache ===========\n");
    fprintf(stderr, "widget %s\n", bioPtr->widgetName);
    fprintf(stderr, "%s\n", title);
    fprintf(stderr, "nbr seqs %d len seqs %d\n", bioPtr->nbSeqsCache, bioPtr->lgSeqsCache);
    for (i=0; i<bioPtr->nbSeqsCache; i++) {
	fprintf(stderr, "%d >%s<\n", i, bioPtr->SeqMatCache[i]); 
    }
    fprintf(stderr, "--------------\n");
    fprintf(stderr, "Tags : nbr %d\n", bioPtr->nbTagsCache);
    for(i=0; i<bioPtr->nbTagsCache;i++) {
	Tcl_ListObjIndex(interp, bioPtr->listTagsCache, i, &tagObj);
	Tcl_GetIntFromObj(NULL, tagObj, &iTag);
	fprintf(stderr, "tag %s i=%d iTag=%d\n", bioPtr->TagsCache[iTag].tagName, i, iTag);
	for(j=0; j<bioPtr->nbSeqsCache; j++) {
	    Tcl_ListObjIndex(interp, bioPtr->TagsCache[iTag].coords, j, &rowObj);
	    Tcl_ListObjLength(interp, rowObj, &len);
	    if (len > 0) {
		fprintf(stderr, "seq= %d ", j);
		for (k=0; k<len; k++) {
		    Tcl_ListObjIndex(NULL, rowObj, k, &debObj);
		    Tcl_GetIntFromObj(interp, debObj, &col);
		    fprintf(stderr, " %d", col);
		}
		fprintf(stderr, "\n");
	    }
	}
    }
    fprintf(stderr, "===============\n");
    fflush(stderr);
	
    return;
}
/* Bug fix: the #endif for "#ifdef DUMPCACHE" (opened above, for
 * DumpCache() only) was misplaced after FontAlreadySeen() instead of
 * right here. That meant FontAlreadySeen() -- called unconditionally
 * from BiotextFontCmd() whenever USE_XFT is defined -- was only ever
 * *defined* when the separate, debug-only DUMPCACHE macro was also
 * defined. In a normal build (DUMPCACHE undefined, which is the
 * common case), this compiled fine (a call to a merely-declared
 * function) but failed to *link*: "undefined reference to
 * FontAlreadySeen". Moved the #endif here so FontAlreadySeen() is
 * always compiled in. */
#endif


static int FontAlreadySeen(char *name, char **list, int count)
{
    for (int i = 0; i < count; i++) {
        if (strcmp(name, list[i]) == 0)
            return 1;
    }
    return 0;
}


#if USE_XFT
static int
FontIsMonospace(Display *display, int screen, const char *family, int pixelsize) 
{
    /* Build a minimal pattern to open the font */
    FcPattern *pattern = FcPatternCreate();
    if (! pattern) {
	return 0;
    }
    
    FcPatternAddString (pattern, FC_FAMILY,     (FcChar8 *)family);
    FcPatternAddDouble (pattern, FC_PIXEL_SIZE, (double)pixelsize);
    FcConfigSubstitute (NULL, pattern, FcMatchPattern);
    FcDefaultSubstitute(pattern);

    FcResult   result;
    FcPattern *match = XftFontMatch(display, screen, pattern, &result);
    FcPatternDestroy(pattern);
    if (! match) {
	return 0;
    }

    XftFont *font = XftFontOpenPattern(display, match);
    if (! font) {
	FcPatternDestroy(match);
	return 0;
    }

    /* Measure advance width of '.' and 'M' */
    XGlyphInfo extents_dot, extents_M;
    XftTextExtentsUtf8(display, font, (FcChar8 *)".", 1, &extents_dot);
    XftTextExtentsUtf8(display, font, (FcChar8 *)"M", 1, &extents_M);

    int is_mono = (extents_dot.xOff == extents_M.xOff);

    XftFontClose(display, font);   /* also frees match */
    return is_mono;
}
#endif


/* End of Biotext widget code */



