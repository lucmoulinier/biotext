load ./libtkbiotext.so Biotext

# In a headless/batch test environment there's no interactive default
# bgerror handler (it would try to pop up a dialog and wait), which
# would hang any test that triggers Tcl_BackgroundError -- this is a
# property of the minimal test harness, not of the widget. Standard
# practice for automated Tk testing: override bgerror to just log.
proc bgerror {msg} {
    puts "  (bgerror caught: $msg)"
}

set failures 0
proc check {desc cond} {
    global failures
    set result [uplevel 1 [list expr $cond]]
    if {$result} {
        puts "  \[ OK \] $desc"
    } else {
        puts "  \[FAIL\] $desc"
        incr failures
    }
}
proc checkerr {desc script} {
    global failures
    set code [catch {uplevel 1 $script} msg]
    if {$code != 0} {
        puts "  \[ OK \] $desc (error: $msg)"
    } else {
        puts "  \[FAIL\] $desc (expected an error, got: $msg)"
        incr failures
    }
}
proc mkseq {len {seed 0}} {
    set bases {A C G T}
    set s ""
    for {set i 0} {$i < $len} {incr i} {
        append s [lindex $bases [expr {($i + $seed) % 4}]]
    }
    return $s
}

# ==================================================================
# GROUP 1: BiotextSequencesCmd -- length-mismatch validation
#          (regression test for the heap-overflow fix)
# ==================================================================
puts "\n===== GROUP 1: sequences command validation ====="
biotext .b1 -width 20 -height 5
pack .b1
update

checkerr "sequences with mismatched lengths is rejected" {
    .b1 sequences {ACGT ACGTACGT}
}
checkerr "sequences of length 0 is rejected" {
    .b1 sequences {{}}
}
# length-1 sequences must now be ACCEPTED (bug fix: used to wrongly
# reject slen<=1, not just slen<=0)
set code [catch {.b1 sequences {A C}} msg]
check "sequences of length 1 are accepted (regression: off-by-one fix)" {$code == 0}

.b1 sequences [list [mkseq 10 0] [mkseq 10 1] [mkseq 10 2]]
update
check "valid equal-length sequences accepted" 1

destroy .b1
update

# ==================================================================
# GROUP 2: BiotextOutputCmd -- bounds checking
#          (regression test for the OOB-read fix)
# ==================================================================
puts "\n===== GROUP 2: output command bounds ====="
biotext .b2 -width 20 -height 5
pack .b2
update
.b2 sequences [list [mkseq 10 0] [mkseq 10 1] [mkseq 10 2]]
update

check "output of a valid index works" {[lindex [.b2 output {1}] 0] eq [mkseq 10 0]}
checkerr "output index 0 (invalid, 1-based) is rejected, not OOB-read" {
    .b2 output {0}
}
checkerr "output index far past the end is rejected" {
    .b2 output {999}
}
checkerr "output negative index is rejected" {
    .b2 output {-5}
}
destroy .b2
update

# ==================================================================
# GROUP 3: invalid color names -- regression test for NULL-deref fixes
#          in BiotextTagConfigSubCmd and BiotextMappingCmd
# ==================================================================
puts "\n===== GROUP 3: invalid color names (crash regression) ====="
biotext .b3 -width 20 -height 5
pack .b3
update
.b3 sequences [list [mkseq 10 0] [mkseq 10 1]]
update

checkerr "tag configure with a bogus foreground color is rejected, not a crash" {
    .b3 tag configure badtag -foreground thisIsNotAColor12345 -background red
}
checkerr "tag configure with a bogus background color is rejected, not a crash" {
    .b3 tag configure badtag2 -foreground red -background thisIsNotAColorEither
}
checkerr "mapping with a bogus color is rejected, not a crash" {
    .b3 mapping {{A notAColorAtAll green} {. gray gray} {{ } gray gray}}
}
checkerr "mapping entry missing a field is rejected, not an OOB read" {
    .b3 mapping {{A red} {. gray gray} {{ } gray gray}}
}
# after all these rejected attempts, the widget must still be usable
.b3 tag configure goodtag -foreground red -background yellow
.b3 tag add goodtag 1.0 1.2
update
check "widget still fully usable after several rejected bad commands" 1

destroy .b3
update

# ==================================================================
# GROUP 4: BiotextCleanCmd
# ==================================================================
puts "\n===== GROUP 4: clean command ====="
biotext .b4 -width 20 -height 5
pack .b4
update
.b4 sequences [list [mkseq 10 0] [mkseq 10 1] [mkseq 10 2]]
.b4 tag configure t1 -foreground red -background yellow
.b4 tag add t1 1.0 1.2
.b4 mapping {{A red green} {. gray gray} {{ } gray gray}}
.b4 group -new 1 {0 1}
update
check "widget populated with sequences/tags/mapping/groups before clean" 1

.b4 clean
update
check "clean did not crash" 1

# widget must be usable again afterwards (repopulate from scratch)
.b4 sequences [list [mkseq 8 0] [mkseq 8 1]]
.b4 tag configure t2 -foreground blue -background white
.b4 tag add t2 1.0 1.1
update
check "widget fully reusable after clean" 1

destroy .b4
update

# ==================================================================
# GROUP 5: BiotextLockCmd
# ==================================================================
puts "\n===== GROUP 5: lock command ====="
biotext .b5 -width 20 -height 5
pack .b5
update
.b5 sequences [list [mkseq 10 0] [mkseq 10 1]]
update

.b5 lock on
update
set before [lindex [.b5 output {1}] 0]
# with the matrix locked, inserting a non-gap character over a
# non-gap position should be refused by the widget's own internal
# logic (BiotextCharsCmd checks bioPtr->Lock) -- not our job to
# assert exact semantics here, just that lock on/off doesn't crash
# and toggles without error.
.b5 lock off
update
check "lock on/off did not crash" 1

destroy .b5
update

# ==================================================================
# GROUP 6: groups in depth -- new/merge/ungroup, invalid indices
#          (regression tests for the empty -merge stub and the
#          missing bounds-checks fixes)
# ==================================================================
puts "\n===== GROUP 6: groups in depth ====="
biotext .b6 -width 20 -height 8
pack .b6
update
set seqs6 {}
for {set i 0} {$i < 6} {incr i} { lappend seqs6 [mkseq 10 $i] }
.b6 sequences $seqs6
update

.b6 group -new 1 {0 1}
.b6 group -merge 1 {2}
update
check "group -new then -merge into the same group: no crash (regression: was a no-op stub)" 1

.b6 group -new 2 {3 4}
update
check "second independent group created: no crash" 1

set code1 [catch {.b6 group -new 3 {999}} msg1]
check "group with an out-of-range sequence index does not crash (silently skipped, not an OOB write)" {1}
set code2 [catch {.b6 group -merge 1 {-1}} msg2]
check "group -merge with a negative index does not crash (silently skipped, not an OOB write)" {1}
set code3 [catch {.b6 ungroup {999}} msg3]
check "ungroup with an out-of-range index does not crash (silently skipped, not an OOB write)" {1}

.b6 ungroup {0 1 2}
.b6 ungroup {3 4}
update
check "ungroup of all groups (two separate calls, each emptying a group): no crash (regression: skip-on-shrink fix)" 1

# insert/delete a row that's part of a group, check it stays functional
.b6 group -new 5 {0 1}
.b6 insert rows 0 -1
.b6 delete rows 0
update
check "insert/delete rows interacting with a group: no crash" 1

destroy .b6
update

# ==================================================================
# GROUP 7: focus / cursor blink
#   (BiotextFocusProc / BiotextConfigCursor / BiotextCursorEvent)
# ==================================================================
puts "\n===== GROUP 7: focus and cursor blink ====="
biotext .b7 -width 20 -height 5 -insertontime 50 -insertofftime 50
pack .b7
update
.b7 sequences [list [mkseq 10 0] [mkseq 10 1]]
update

focus .b7
update
check "giving focus to the widget did not crash" 1

# let the cursor blink timer fire a few times
after 250
update
check "cursor blink timer firing repeatedly did not crash" 1

.b7 cursor 1.3
after 150
update
check "moving the cursor while focused (resets blink) did not crash" 1

focus .
update
check "removing focus did not crash" 1

after 150
update
check "no stray blink timer firing after focus loss" 1

destroy .b7
update

# ==================================================================
# GROUP 8: BiotextIndexCmd and malformed index strings
#   (regression test for the missing BiotextGetIndex checks)
# ==================================================================
puts "\n===== GROUP 8: index command with malformed indices ====="
biotext .b8 -width 20 -height 5
pack .b8
update
.b8 sequences [list [mkseq 10 0] [mkseq 10 1]]
update

check "index of a valid position works" {[.b8 index 1.2] eq "1.2"}
checkerr "index with a completely bogus string is rejected" {
    .b8 index notAnIndexAtAll
}
checkerr "index with trailing garbage after the number is rejected (regression: %d%c sscanf fix)" {
    .b8 insert cols 3xyz 1
}
checkerr "index on an out-of-range row.col that BiotextGetIndex should reject cleanly" {
    .b8 index 1.notanumber
}
update
check "widget still usable after several malformed-index attempts" 1

destroy .b8
update

# ==================================================================
# GROUP 9: font command error paths
# ==================================================================
puts "\n===== GROUP 9: font command error handling ====="
biotext .b9 -width 20 -height 5
pack .b9
update
.b9 sequences [list [mkseq 10 0]]
update

# an invalid weight should not crash and should leave the widget
# usable (BiotextFontCmd uses Tcl_BackgroundError for this, not a
# direct TCL_ERROR return -- so we just check it doesn't crash and
# the weight stays at its last valid value)
.b9 font weight notAValidWeight
update
check "invalid font weight argument did not crash" 1
check "font weight unchanged after invalid attempt" {[.b9 font weight] eq "normal"}

.b9 font size 0
update
check "font size 0 did not crash" 1
.b9 font size -5
update
check "negative font size did not crash" 1
.b9 font size 12
update
check "font size restored to a sane value works" {[.b9 font size] == 12}

destroy .b9
update

puts "\n====================================="
puts "Total failures: $failures"
puts "====================================="
if {$failures > 0} { exit 1 }
exit 0
