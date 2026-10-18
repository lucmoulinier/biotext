# biotext

Tk widget to display and edit multiple sequence alignments, used by the Ordali alignment editor.
Package `biotext` 1.0. It builds against Tcl/Tk 8.6 and Tcl/Tk 9 from the same sources.

```tcl
package require biotext
biotext .b
pack .b
```

Source layout: `generic/` (widget), `unix/` (X11 drawing with Xft), `win/` (Windows), `macos/` (Aqua),
`library/biotext.tcl` (bindings).

## Build

TEA (autoconf) build, out of the source tree. Tk is required; on Unix, Xft and fontconfig
(`libxft-dev libfontconfig1-dev`) are also needed.

```sh
mkdir build-tcl8 && cd build-tcl8
../configure --with-tcl=/path/to/tcl8.6/lib --with-tk=/path/to/tk8.6/lib && make   # -> libbiotext1.0.so
mkdir build-tcl9 && cd build-tcl9
../configure --with-tcl=/path/to/tcl9/lib --with-tk=/path/to/tk9/lib && make       # -> libtcl9biotext1.0.so
make install
```

On Windows, build with MinGW/MSYS2 the same way.
`configure` is already generated; run `autoconf` only after editing `configure.ac`.

## Tests

The tests are in `tests/` (`tests/bugs.txt`, in French, lists the bugs fixed so far):

- C logic tests, no display needed: `sh tests/build_and_run_tests.sh`
  (`tkBiotext_extracted_functions.c` is a copy of the editing functions of `generic/tkBiotext.c`;
  keep it in sync).
- Widget tests, need an X display: build `tests/libtkbiotext.so` (or link it to a TEA build), then
  `cd tests && wish test_widget.tcl` (also `test_viewport.tcl`, `test_misc.tcl`, `test_more.tcl`,
  `test_advanced.tcl`, `test_realistic_msa.tcl`, `test_tag_insert_push.tcl`).
  `sh tests/build_real_widget.sh` builds the library and runs the first two under Xvfb.

## License

GPL-3.0-or-later, Copyright (c) Luc Moulinier. See [LICENSE](LICENSE).
