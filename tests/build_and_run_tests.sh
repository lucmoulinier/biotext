#!/bin/sh
# Build & run the biotext test harness.
#
# This test harness does NOT reimplement the widget's logic: it links
# and calls the REAL BiotextInsertCols / BiotextDeleteCols /
# BiotextInsertRows / BiotextDeleteRows / BiotextCopyCmd / BiotextCutCmd /
# BiotextPasteCmd / BiotextInsertGapsInTags / cache helper functions,
# extracted verbatim from the patched tkBiotext.c (see
# tkBiotext_extracted_functions.c), against a real Tcl interpreter.
# No Tk/X11 is required: none of these functions touch the display.
#
# Requirements: tcl8.6-dev (headers + library), and optionally valgrind
# to check for heap corruption / leaks in addition to the logical
# assertions.
#
#   sudo apt-get install -y tcl8.6-dev valgrind
#
set -e
SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
cd "$SCRIPT_DIR"

TCL_INC=$(find /usr/include -maxdepth 1 -iname "tcl8.6" | head -1)
if [ -z "$TCL_INC" ]; then
    echo "tcl8.6 headers not found. Install with: sudo apt-get install tcl8.6-dev" >&2
    exit 1
fi

cat test_biotext_min.h tkBiotext_extracted_functions.c test_biotext.c > combined_test.c

gcc -std=gnu11 -g -O0 -Wall -Wno-unused-but-set-variable -Wno-unused-function \
    -I"$TCL_INC" combined_test.c -ltcl8.6 -lm -o test_biotext

echo "== Running logical tests =="
./test_biotext
STATUS=$?

if command -v valgrind >/dev/null 2>&1; then
    echo
    echo "== Running under valgrind (memory-safety check) =="
    echo "   (definitely/indirectly lost bytes and any invalid access are the"
    echo "    signal to watch; 'possibly lost'/'still reachable' blocks coming"
    echo "    from Tcl_CreateInterp's own internal caches are expected noise)"
    valgrind --leak-check=full --show-leak-kinds=all \
             --errors-for-leak-kinds=definite,indirect \
             --track-origins=yes --error-exitcode=99 ./test_biotext >/dev/null
    echo "valgrind: no invalid access, no definite/indirect leak detected."
fi

exit $STATUS
