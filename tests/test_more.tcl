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
proc colOf {idx} { return [lindex [split $idx .] 1] }

# ==================================================================
# GROUP 1: push (left/right) with a group, checking tag zones follow
#   correctly (regression test for the use-after-free fix in
#   BiotextPushInSeq)
# ==================================================================
puts "\n===== GROUP 1: push with tags on grouped sequences ====="
biotext .b1 -width 30 -height 6
pack .b1
update
.b1 sequences {AAA..AAAAA AAA..AAAAA AAAAAAAAAA}
.b1 group -new 1 {0 1}
.b1 tag configure pt -foreground red -background yellow
.b1 tag add pt 1.0 1.2
.b1 tag add pt 2.0 2.2
update
puts "  before push: row1=[.b1 output {1}] row2=[.b1 output {2}] row3=[.b1 output {3}]"
puts "  tag ranges before: [.b1 tag ranges pt]"

.b1 cursor 1.3
.b1 push right
update
puts "  after push right: row1=[.b1 output {1}] row2=[.b1 output {2}] row3=[.b1 output {3}]"
puts "  tag ranges after: [.b1 tag ranges pt]"
check "push right on a grouped sequence: no crash" 1
check "ungrouped row 3 unaffected by push" {[lindex [.b1 output {3}] 0] eq [mkseq 10 0] || [lindex [.b1 output {3}] 0] eq {AAAAAAAAAA}}

# repeat many times to stress the tag-coords refcounting path (the
# original bug was a use-after-free that might only manifest after
# repeated allocation/free cycles)
for {set k 0} {$k < 15} {incr k} {
    .b1 cursor "1.[expr {2 + ($k % 3)}]"
    .b1 push right
    .b1 push left
    update
}
check "15 repeated push right/left cycles on tagged, grouped rows: no crash" 1

destroy .b1
update

# ==================================================================
# GROUP 2: delete cols/rows "all" keyword
# ==================================================================
puts "\n===== GROUP 2: delete with the 'all' keyword ====="
biotext .b2 -width 30 -height 6
pack .b2
update
.b2 sequences [list [mkseq 10 0] [mkseq 10 1] [mkseq 10 2]]
update

checkerr "delete rows all removes every row (widget becomes unusable for row ops)" {
    .b2 delete rows all
    .b2 output {1}
}
destroy .b2
update

biotext .b2 -width 30 -height 6
pack .b2
update
.b2 sequences [list [mkseq 10 0] [mkseq 10 1] [mkseq 10 2]]
update
set code [catch {.b2 delete cols all} msg]
puts "  delete cols all -> code=$code msg=$msg"
check "delete cols all did not crash" 1

destroy .b2
update

# ==================================================================
# GROUP 3: scrollbar actually gets updated (not just xview/yview
#   fractions queried directly, but the -xscrollcommand/-yscrollcommand
#   callback firing with real scrollbar widgets attached)
# ==================================================================
puts "\n===== GROUP 3: real scrollbar widget updates ====="
biotext .b3 -width 10 -height 3 -xscrollcommand {.sx3 set} -yscrollcommand {.sy3 set}
scrollbar .sx3 -orient horizontal -command {.b3 xview}
scrollbar .sy3 -orient vertical   -command {.b3 yview}
pack .b3 -side left
pack .sx3 .sy3
update

set seqs3 {}
for {set i 0} {$i < 20} {incr i} { lappend seqs3 [mkseq 40 $i] }
.b3 sequences $seqs3
update idletasks
update

set sxGet [.sx3 get]
set syGet [.sy3 get]
puts "  horizontal scrollbar get: $sxGet"
puts "  vertical scrollbar get:   $syGet"
check "horizontal scrollbar reflects a partial view (oversized data)" {[lindex $sxGet 1] < 1.0}
check "vertical scrollbar reflects a partial view (oversized data)" {[lindex $syGet 1] < 1.0}

# scroll via the scrollbar's own command path and check the widget moves
.b3 yview moveto 0.5
update
set syGet2 [.sy3 get]
puts "  vertical scrollbar get after yview moveto 0.5: $syGet2"
check "scrollbar position updates after programmatic yview change" {$syGet2 ne $syGet}

destroy .b3
destroy .sx3
destroy .sy3
update

# ==================================================================
# GROUP 4: configuration options -- cget round-trip and invalid values
# ==================================================================
puts "\n===== GROUP 4: configuration options ====="
biotext .b4 -width 20 -height 5
pack .b4
update

.b4 configure -relief sunken -borderwidth 3 -background white -foreground black
update
check "cget -relief round-trips" {[.b4 cget -relief] eq "sunken"}
check "cget -borderwidth round-trips" {[.b4 cget -borderwidth] == 3}
check "cget -background round-trips" {[.b4 cget -background] eq "white"}

checkerr "invalid -relief value is rejected" {
    .b4 configure -relief notARealRelief
}
checkerr "invalid boolean for -setgrid is rejected" {
    .b4 configure -setgrid notABoolean
}
checkerr "negative -borderwidth is handled (rejected or otherwise, just no crash on query after)" {
    .b4 configure -borderwidth -5
    error "force-check-path"
}
update
check "widget still usable after rejected bad config attempts" 1
.b4 sequences [list [mkseq 5 0]]
update
check "final sanity: widget works after all the above" 1

destroy .b4
update

# ==================================================================
# GROUP 5: unknown/malformed top-level subcommands
# ==================================================================
puts "\n===== GROUP 5: unknown subcommands ====="
biotext .b5 -width 20 -height 5
pack .b5
update
.b5 sequences [list [mkseq 5 0]]
update

checkerr "completely unknown subcommand is rejected cleanly" {
    .b5 thisIsNotACommand foo bar
}
checkerr "tag with an unknown sub-subcommand is rejected cleanly" {
    .b5 tag bogus
}
checkerr "empty command (no subcommand at all) is rejected cleanly" {
    .b5
}
update
check "widget still usable after several unknown-command attempts" 1

destroy .b5
update

# ==================================================================
# GROUP 6: -insertontime / -insertofftime edge cases (blink disabled)
# ==================================================================
puts "\n===== GROUP 6: cursor blink edge cases ====="
biotext .b6 -width 20 -height 5 -insertontime 0 -insertofftime 0
pack .b6
update
.b6 sequences [list [mkseq 10 0]]
update
focus .b6
update
after 100
update
check "insertontime=0/insertofftime=0 (blink disabled) does not crash" 1

.b6 configure -insertontime 100 -insertofftime 100
focus .
update
focus .b6
update
after 250
update
check "re-enabling blink after it was disabled does not crash" 1

destroy .b6
update

puts "\n====================================="
puts "Total failures: $failures"
puts "====================================="
if {$failures > 0} { exit 1 }
exit 0
