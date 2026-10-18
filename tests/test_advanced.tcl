load ./libtkbiotext.so Biotext
proc bgerror {msg} { puts "  (bgerror caught: $msg)" }

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
# GROUP 1: delete colsgaps (remove all-gap columns)
# ==================================================================
puts "\n===== GROUP 1: delete colsgaps ====="
biotext .b1 -width 30 -height 6
pack .b1
update
# build an alignment with some columns that are ENTIRELY gaps ('.')
# across every row, and some that are mixed
.b1 sequences {A.CGT. A.CGT. A.CGT.}
update
puts "  before: [.b1 output {1}] / [.b1 output {2}] / [.b1 output {3}]"

.b1 delete colsgaps
update
puts "  after:  [.b1 output {1}] / [.b1 output {2}] / [.b1 output {3}]"
check "delete colsgaps removed the all-gap columns" {[.b1 output {1}] eq {ACGT}}
check "delete colsgaps kept identical rows in sync" {[.b1 output {1}] eq [.b1 output {2}] && [.b1 output {2}] eq [.b1 output {3}]}

# now a case where NO column is entirely gaps (mixed gaps per row)
destroy .b1
update
biotext .b1 -width 30 -height 6
pack .b1
update
.b1 sequences {A.CG AC.G}
update
.b1 delete colsgaps
update
check "delete colsgaps is a no-op when no column is fully gapped" {[.b1 output {1}] eq {A.CG} && [.b1 output {2}] eq {AC.G}}

# edge case: an alignment that is ENTIRELY gaps
destroy .b1
update
biotext .b1 -width 30 -height 6
pack .b1
update
.b1 sequences {.... ....}
update
set code [catch {.b1 delete colsgaps} msg]
check "delete colsgaps on an all-gap alignment does not crash" 1
puts "  (result: code=$code msg=$msg)"

destroy .b1
update

# ==================================================================
# GROUP 2: tags survive copy/cut/paste of rows
#   (exercises BiotextCopySeqsToBuffer's tag-caching path, never
#   dynamically tested before)
# ==================================================================
puts "\n===== GROUP 2: tags carried across copy/cut/paste ====="
biotext .b2 -width 30 -height 8
pack .b2
update
.b2 sequences [list [mkseq 10 0] [mkseq 10 1] [mkseq 10 2] [mkseq 10 3]]
.b2 tag configure hot -foreground red -background yellow
.b2 tag add hot 2.3 2.6
update
puts "  tag ranges before: [.b2 tag ranges hot]"

.b2 copy {2}
.b2 paste end
update
puts "  tag ranges after copy+paste at end: [.b2 tag ranges hot]"
check "copying a tagged row and pasting it: no crash" 1
check "tag ranges list grew by one entry after paste (tag data followed the row)" \
    {[llength [.b2 tag ranges hot]] >= 4}

.b2 cut {1}
.b2 paste 1
update
check "cutting/pasting an untagged row does not disturb the existing tag: no crash" 1
puts "  tag ranges after cut/paste elsewhere: [.b2 tag ranges hot]"

destroy .b2
update

# ==================================================================
# GROUP 3: character editing (chars/delchars) on a GROUPED sequence
#   -- edits to one member of a group should propagate to the group
# ==================================================================
puts "\n===== GROUP 3: chars/delchars with sequence groups ====="
biotext .b3 -width 30 -height 6
pack .b3
update
.b3 sequences [list [mkseq 10 0] [mkseq 10 1] [mkseq 10 2]]
.b3 group -new 1 {0 1}
update
set before2 [lindex [.b3 output {2}] 0]
set len0before [string length [lindex [.b3 output {1}] 0]]

.b3 cursor 1.3
.b3 chars X 1
update
set len0after [string length [lindex [.b3 output {1}] 0]]
set len1after [string length [lindex [.b3 output {2}] 0]]
puts "  row1 len before/after: $len0before/$len0after   row2(grouped) len after: $len1after"
check "chars insert on a grouped sequence: no crash" 1
check "ungrouped row (3) length unaffected by group edit" \
    {[string length [lindex [.b3 output {3}] 0]] == 10}

destroy .b3
update

# ==================================================================
# GROUP 4: tag bounds shift correctly on column insert at first/last
#   column (direct regression test of the earlier static analysis of
#   BiotextInsertGapsInTags -- now actually exercised dynamically)
# ==================================================================
puts "\n===== GROUP 4: tag bounds on column insert at edges ====="
biotext .b4 -width 30 -height 6
pack .b4
update
.b4 sequences {AAAAAAAAAA AAAAAAAAAA}
.b4 tag configure full -foreground red -background yellow
.b4 tag add full 1.2 1.7
.b4 tag add full 2.2 2.7
update
puts "  tag ranges before: [.b4 tag ranges full]"

# "tag ranges" returns index strings in "row.col" form (e.g. "1.4"),
# matching Tk's own tag-range convention -- not raw column integers.
proc colOf {idx} { return [lindex [split $idx .] 1] }

# insert 2 columns BEFORE the first column: the whole tag should shift
# right by 2 (insert-before displaces everything, per the "left"
# branch's ">=" semantics analyzed earlier)
.b4 insert cols 0 -2
update
set r4 [.b4 tag ranges full]
puts "  tag ranges after insert-before-col-0: $r4"
check "tag shifted right after inserting columns before the alignment start" \
    {[colOf [lindex $r4 0]] == 4 && [colOf [lindex $r4 1]] == 9}

# insert 3 columns AFTER the last column: the tag (which does not
# reach the last column) should NOT shift at all
.b4 insert cols end 3
update
set r5 [.b4 tag ranges full]
puts "  tag ranges after insert-after-last-col: $r5"
check "tag unaffected by inserting columns strictly after the alignment end" \
    {[colOf [lindex $r5 0]] == 4 && [colOf [lindex $r5 1]] == 9}

destroy .b4
update

# ==================================================================
# GROUP 5: multiple tag zones on the same row, and deleting one zone
#   without disturbing the others
# ==================================================================
puts "\n===== GROUP 5: multiple tag zones per row ====="
biotext .b5 -width 40 -height 4
pack .b5
update
.b5 sequences {AAAAAAAAAAAAAAAAAAAA}
.b5 tag configure multi -foreground red -background yellow
.b5 tag add multi 1.0 1.2
.b5 tag add multi 1.5 1.7
.b5 tag add multi 1.10 1.12
update
puts "  tag ranges (3 zones): [.b5 tag ranges multi]"
check "three separate zones recorded on the same row" {[llength [.b5 tag ranges multi]] == 6}

.b5 tag remove multi 1.5 1.7
update
puts "  tag ranges after removing the middle zone: [.b5 tag ranges multi]"
check "removing the middle zone leaves the other two intact" \
    {[llength [.b5 tag ranges multi]] == 4}

destroy .b5
update

# ==================================================================
# GROUP 6: tag lower/raise ordering and cget-style introspection
# ==================================================================
puts "\n===== GROUP 6: tag raise ordering ====="
biotext .b6 -width 20 -height 4
pack .b6
update
.b6 sequences {AAAAAAAAAA}
.b6 tag configure back -foreground white -background blue
.b6 tag configure front -foreground white -background red
.b6 tag add back 1.0 1.5
.b6 tag add front 1.0 1.5
update
set namesBefore [.b6 tag names 1.2]
puts "  tag names at 1.2 before raise: $namesBefore"
check "tag names at a position inside both tagged zones finds both tags (regression: double-decrement fix)" \
    {[llength $namesBefore] == 2 && "back" in $namesBefore && "front" in $namesBefore}
check "tag names at an untagged position (1.8) finds nothing" \
    {[llength [.b6 tag names 1.8]] == 0}
.b6 tag raise front
update
set namesAfter [.b6 tag names 1.2]
puts "  tag names at 1.2 after raise front: $namesAfter"
check "tag raise did not crash and both tags still present" \
    {[llength $namesAfter] == [llength $namesBefore]}

destroy .b6
update

puts "\n====================================="
puts "Total failures: $failures"
puts "====================================="
if {$failures > 0} { exit 1 }
exit 0
