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
proc colOf {idx} { return [lindex [split $idx .] 1] }
proc rowOf {idx} { return [lindex [split $idx .] 0] }

# ==================================================================
# GROUP 1: single row, tag in the middle, insert a column INSIDE it,
#   then push left/right (the user's exact suggested scenario)
# ==================================================================
puts "\n===== GROUP 1: tag zone, insert in the middle, then push ====="
biotext .b1 -width 30 -height 6
pack .b1
update
.b1 sequences {AAAAAAAAAA}
.b1 tag configure mid -foreground red -background yellow
.b1 tag add mid 1.2 1.7
update
set r0 [.b1 tag ranges mid]
puts "  tag before: $r0 ; row: [lindex [.b1 output {1}] 0]"
check "tag correctly set up before insert" {[colOf [lindex $r0 0]] == 2 && [colOf [lindex $r0 1]] == 7}

# insert 1 column strictly inside the tag zone (after column 4)
.b1 insert cols 4 1
update
set r1 [.b1 tag ranges mid]
set row1 [lindex [.b1 output {1}] 0]
puts "  tag after insert-in-middle: $r1 ; row: $row1"
check "tag GREW by the inserted gap count (still covers the same logical span)" \
    {[colOf [lindex $r1 0]] == 2 && [colOf [lindex $r1 1]] == 8}
check "row length grew by 1 and the new gap sits inside the (now-wider) tag zone" \
    {[string length $row1] == 11 && [string index $row1 5] eq "."}

# push right from inside the tag: must not crash, tag bounds must stay
# a valid, well-formed zone (start <= stop, both within row length)
.b1 cursor 1.4
.b1 push right
update
set r2 [.b1 tag ranges mid]
set row2 [lindex [.b1 output {1}] 0]
puts "  tag after push right: $r2 ; row: $row2"
check "push right inside the tag: no crash" 1
check "tag zone still well-formed after push right (start <= stop)" \
    {[colOf [lindex $r2 0]] <= [colOf [lindex $r2 1]]}
check "row length unchanged by push (push only moves a gap, doesn't insert/delete)" \
    {[string length $row2] == 11}

# push left back
.b1 cursor 1.4
.b1 push left
update
set r3 [.b1 tag ranges mid]
set row3 [lindex [.b1 output {1}] 0]
puts "  tag after push left: $r3 ; row: $row3"
check "push left inside the tag: no crash" 1
check "tag zone still well-formed after push left (start <= stop)" \
    {[colOf [lindex $r3 0]] <= [colOf [lindex $r3 1]]}

destroy .b1
update

# ==================================================================
# GROUP 2: SAME scenario but with MULTIPLE ROWS, all tagged
#   (direct stress test of the row-index off-by-one fix, bug 40)
# ==================================================================
puts "\n===== GROUP 2: multi-row tag zones, insert in the middle, then push ====="
biotext .b2 -width 30 -height 8
pack .b2
update
.b2 sequences {AAAAAAAAAA AAAAAAAAAA AAAAAAAAAA AAAAAAAAAA}
.b2 tag configure mid -foreground red -background yellow
for {set r 1} {$r <= 4} {incr r} {
    .b2 tag add mid $r.2 $r.7
}
update
set rb [.b2 tag ranges mid]
puts "  tag before (4 rows): $rb"
check "all 4 rows have identical tag bounds before insert" {
    [colOf [lindex $rb 0]] == 2 && [colOf [lindex $rb 1]] == 7 &&
    [colOf [lindex $rb 2]] == 2 && [colOf [lindex $rb 3]] == 7 &&
    [colOf [lindex $rb 4]] == 2 && [colOf [lindex $rb 5]] == 7 &&
    [colOf [lindex $rb 6]] == 2 && [colOf [lindex $rb 7]] == 7
}

.b2 insert cols 4 1
update
set ra [.b2 tag ranges mid]
puts "  tag after insert-in-middle (4 rows): $ra"
check "ALL 4 rows shifted identically after inserting in the middle (regression: bug 40)" {
    [colOf [lindex $ra 0]] == 2 && [colOf [lindex $ra 1]] == 8 &&
    [colOf [lindex $ra 2]] == 2 && [colOf [lindex $ra 3]] == 8 &&
    [colOf [lindex $ra 4]] == 2 && [colOf [lindex $ra 5]] == 8 &&
    [colOf [lindex $ra 6]] == 2 && [colOf [lindex $ra 7]] == 8
}

# push right/left on every row, one at a time
for {set r 1} {$r <= 4} {incr r} {
    .b2 cursor $r.4
    .b2 push right
    .b2 cursor $r.4
    .b2 push left
}
update
set rp [.b2 tag ranges mid]
puts "  tag after push on all 4 rows: $rp"
check "push on every row: no crash, all 4 zones still well-formed" {
    [colOf [lindex $rp 0]] <= [colOf [lindex $rp 1]] &&
    [colOf [lindex $rp 2]] <= [colOf [lindex $rp 3]] &&
    [colOf [lindex $rp 4]] <= [colOf [lindex $rp 5]] &&
    [colOf [lindex $rp 6]] <= [colOf [lindex $rp 7]]
}

destroy .b2
update

# ==================================================================
# GROUP 3: insert exactly AT the tag's start/end boundary (not
#   strictly inside) -- the sharper edge cases of the >/>= logic
# ==================================================================
puts "\n===== GROUP 3: insert exactly at the tag boundary (start/end) ====="
biotext .b3 -width 30 -height 6
pack .b3
update
.b3 sequences {AAAAAAAAAA}
.b3 tag configure edge -foreground red -background yellow
.b3 tag add edge 1.3 1.6
update
puts "  tag before: [.b3 tag ranges edge]"

# insert exactly at the tag's start column (3)
.b3 insert cols 3 1
update
set re1 [.b3 tag ranges edge]
puts "  tag after insert AT start col (3): $re1"
check "insert at the tag's own start column: no crash, zone still well-formed" \
    {[colOf [lindex $re1 0]] <= [colOf [lindex $re1 1]]}

destroy .b3
update
biotext .b3 -width 30 -height 6
pack .b3
update
.b3 sequences {AAAAAAAAAA}
.b3 tag configure edge -foreground red -background yellow
.b3 tag add edge 1.3 1.6
update
# insert exactly at the tag's end column (6)
.b3 insert cols 6 1
update
set re2 [.b3 tag ranges edge]
puts "  tag after insert AT end col (6): $re2"
check "insert at the tag's own end column: no crash, zone still well-formed" \
    {[colOf [lindex $re2 0]] <= [colOf [lindex $re2 1]]}

destroy .b3
update

# ==================================================================
# GROUP 4: delete a column from INSIDE a tagged zone, then push
# ==================================================================
puts "\n===== GROUP 4: delete a column from inside the tag, then push ====="
biotext .b4 -width 30 -height 6
pack .b4
update
.b4 sequences {AAAAAAAAAA AAAAAAAAAA}
.b4 tag configure mid -foreground red -background yellow
.b4 tag add mid 1.2 1.7
.b4 tag add mid 2.2 2.7
update
puts "  tag before: [.b4 tag ranges mid]"

.b4 delete cols 4
update
set rd [.b4 tag ranges mid]
puts "  tag after delete col 4 (inside zone, both rows): $rd"
check "deleting a column from inside the tag shrinks it by one (both rows, regression: bug 40)" {
    [colOf [lindex $rd 0]] == 2 && [colOf [lindex $rd 1]] == 6 &&
    [colOf [lindex $rd 2]] == 2 && [colOf [lindex $rd 3]] == 6
}

.b4 cursor 1.3
.b4 push right
.b4 cursor 2.3
.b4 push left
update
check "push after a mid-zone column delete: no crash" 1

destroy .b4
update

puts "\n====================================="
puts "Total failures: $failures"
puts "====================================="
if {$failures > 0} { exit 1 }
exit 0
