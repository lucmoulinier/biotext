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

proc allRowsSameLength {w nrows} {
    set len0 [string length [lindex [$w output {1}] 0]]
    for {set r 2} {$r <= $nrows} {incr r} {
        if {[string length [lindex [$w output $r] 0]] != $len0} {
            return 0
        }
    }
    return 1
}

proc tagWellFormed {ranges} {
    foreach {a b} $ranges {
        if {[colOf $a] > [colOf $b]} { return 0 }
    }
    return 1
}

# ==================================================================
# Realistic multiple alignment: gaps scattered throughout (not just
# at the edges), 3 grouped sequences + 1 ungrouped, two tags -- one
# ("core") spanning a region that straddles internal gaps on every
# row, one ("outside") only on the ungrouped row, away from "core".
# ==================================================================
puts "\n===== Realistic MSA: grouped+ungrouped rows, tags in/out of push zone ====="
biotext .b -width 40 -height 8
pack .b
update

.b sequences {
    MK--VLSTQ-GHK
    MKAAVL--QRGHK
    -KAAVLSTQRG-K
    MKAAVLST--GHK
}
update
for {set r 1} {$r <= 4} {incr r} {
    puts "  row $r before: [lindex [.b output $r] 0]"
}
check "realistic gapped alignment loaded, all rows equal length" {[allRowsSameLength .b 4]}

.b group -new 1 {0 1 2}
.b tag configure core -foreground red -background yellow
for {set r 1} {$r <= 4} {incr r} {
    .b tag add core $r.3 $r.9
}
.b tag configure outside -foreground blue -background white
.b tag add outside 4.0 4.1
update
set coreBefore [.b tag ranges core]
check "core tag spans a gap-straddling zone on all 4 rows, well-formed" \
    {[llength $coreBefore] == 8 && [tagWellFormed $coreBefore]}

# ---- push RIGHT inside the tag zone, inside the group ----
.b cursor 1.5
.b push right
update
check "push right inside tag+group: alignment width invariant holds (all rows still equal length)" \
    {[allRowsSameLength .b 4]}
set coreAfter1 [.b tag ranges core]
set outsideAfter1 [.b tag ranges outside]
check "core tag remains well-formed after push right inside it" {[tagWellFormed $coreAfter1]}
check "outside tag (on the ungrouped row, untouched region) unchanged by a push on the group" \
    {$outsideAfter1 eq {4.0 4.1}}
for {set r 1} {$r <= 4} {incr r} {
    puts "  row $r after push-right(1.5): [lindex [.b output $r] 0]"
}

# ---- push LEFT to try to bring it back ----
.b cursor 1.5
.b push left
update
check "push left after push right: no crash" 1
check "alignment width invariant still holds after push left" {[allRowsSameLength .b 4]}
check "core tag still well-formed after push left" {[tagWellFormed [.b tag ranges core]]}

# ---- push RIGHT on the UNGROUPED row, inside the 'outside' tag,
#      away from 'core' ----
.b cursor 4.0
.b push right
update
check "push right on ungrouped row inside a different, unrelated tag: no crash" 1
check "alignment width invariant holds after push on the ungrouped row" {[allRowsSameLength .b 4]}
set coreAfter2 [.b tag ranges core]
set outsideAfter2 [.b tag ranges outside]
check "core tag (rows 1-3, untouched by this push) still well-formed" {[tagWellFormed $coreAfter2]}
check "outside tag shifted correctly and remains well-formed after the push that touched it" \
    {[tagWellFormed $outsideAfter2] && $outsideAfter2 ne {4.0 4.1}}
for {set r 1} {$r <= 4} {incr r} {
    puts "  row $r after push-right(4.0): [lindex [.b output $r] 0]"
}
puts "  core: [.b tag ranges core]   outside: [.b tag ranges outside]"

# ---- push LEFT deep inside the group, at a position with a gap
#      immediately adjacent (stresses BiotextFindNextGap on a
#      realistic, densely-gapped sequence) ----
.b cursor 2.6
.b push left
.b cursor 3.10
.b push right
update
check "push left/right at various realistic gap-adjacent positions: no crash" 1
check "alignment width invariant holds after further pushes" {[allRowsSameLength .b 4]}
check "core tag still well-formed after further pushes" {[tagWellFormed [.b tag ranges core]]}

# ---- stress: many pushes in a row across the whole group, mixing
#      directions and rows, on this realistic dataset ----
for {set k 0} {$k < 20} {incr k} {
    set row [expr {1 + ($k % 3)}]
    set col [expr {2 + ($k % 9)}]
    .b cursor "$row.$col"
    if {$k % 2 == 0} { .b push right } else { .b push left }
}
update
check "20 mixed push operations across the grouped rows: no crash" 1
check "alignment width invariant holds after the push stress loop" {[allRowsSameLength .b 4]}
check "core tag still well-formed after the push stress loop" {[tagWellFormed [.b tag ranges core]]}
check "outside tag still well-formed after the push stress loop" {[tagWellFormed [.b tag ranges outside]]}

destroy .b
update

puts "\n====================================="
puts "Total failures: $failures"
puts "====================================="
if {$failures > 0} { exit 1 }
exit 0
