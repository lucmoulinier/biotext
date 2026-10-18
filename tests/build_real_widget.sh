#!/bin/sh
# Build the REAL biotext widget as a loadable Tcl package (.so) and run
# the end-to-end smoke test (test_widget.tcl) against it.
#
# Requirements (Debian/Ubuntu):
#   sudo apt-get install -y tcl8.6-dev tk8.6-dev libxft-dev \
#       libfontconfig1-dev xvfb valgrind
#
set -e
SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
cd "$SCRIPT_DIR"

mkdir -p generic unix
cp tkBiotext.h tkBiotext.c generic/
cp tkBioUnix.c unix/

TCL_INC=/usr/include/tcl8.6
TK_PRIV_UNIX=$TCL_INC/tk-private/unix
TK_PRIV_GEN=$TCL_INC/tk-private/generic
FT2_INC=$(pkg-config --cflags freetype2 2>/dev/null || echo "-I/usr/include/freetype2 -I/usr/include/libpng16")

CFLAGS="-std=gnu11 -g -O0 -Wall -fPIC -DUSE_XFT=1 -DDLLEXPORT= -I$TCL_INC $FT2_INC"

gcc -c $CFLAGS generic/tkBiotext.c -o generic/tkBiotext.o
gcc -c $CFLAGS -I"$TK_PRIV_UNIX" -I"$TK_PRIV_GEN" unix/tkBioUnix.c -o unix/tkBioUnix.o

gcc -shared -fPIC -o libtkbiotext.so unix/tkBioUnix.o generic/tkBiotext.o \
    -ltcl8.6 -ltk8.6 -lXft -lfontconfig -lX11

echo "== Build OK: libtkbiotext.so =="

# Run under a virtual X display (no real screen needed)
rm -f /tmp/.X99-lock
Xvfb :99 -screen 0 1024x768x24 &
XVFB_PID=$!
sleep 2

echo "== Running end-to-end smoke test =="
DISPLAY=:99 wish8.6 test_widget.tcl
STATUS=$?

if [ -f test_viewport.tcl ]; then
    echo
    echo "== Running viewport/scaling test suite (small vs. oversized data, multiple widgets, off-screen ops) =="
    DISPLAY=:99 wish8.6 test_viewport.tcl
    STATUS2=$?
    if [ $STATUS2 -ne 0 ]; then STATUS=$STATUS2; fi
fi

if command -v valgrind >/dev/null 2>&1; then
    echo
    echo "== Running under valgrind (memory-safety check) =="
    echo "   (fontconfig/expat startup leaks are expected noise, present"
    echo "    in any Xft-based app; only errors touching tkBiotext.c/"
    echo "    tkBioUnix.c in the stack trace would be a real problem)"
    DISPLAY=:99 valgrind --leak-check=full --show-leak-kinds=all \
        --errors-for-leak-kinds=definite,indirect --track-origins=yes \
        wish8.6 test_widget.tcl > /dev/null
    if [ -f test_viewport.tcl ]; then
        DISPLAY=:99 valgrind --leak-check=full --show-leak-kinds=all \
            --errors-for-leak-kinds=definite,indirect --track-origins=yes \
            wish8.6 test_viewport.tcl > /dev/null
    fi
fi

kill $XVFB_PID 2>/dev/null
exit $STATUS
