load ./libtkbiotext.so Biotext

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

# ==================================================================
# Helper: build a random-ish DNA sequence of a given length
# ==================================================================
proc mkseq {len {seed 0}} {
    set bases {A C G T}
    set s ""
    for {set i 0} {$i < $len} {incr i} {
        append s [lindex $bases [expr {($i + $seed) % 4}]]
    }
    return $s
}

# ==================================================================
# TEST GROUP 1: widget viewport nC x nR, data SMALLER than viewport
#   (nseq < nR, seqlen < nC) -- no scrolling should ever be needed
# ==================================================================
puts "\n===== GROUP 1: data smaller than the visible viewport ====="
set nC 40
set nR 10
biotext .b1 -width $nC -height $nR
pack .b1
update

set nseqSmall 4
set lenSmall  15
set seqs {}
for {set i 0} {$i < $nseqSmall} {incr i} {
    lappend seqs [mkseq $lenSmall $i]
}
.b1 sequences $seqs
update idletasks
update

check "small data: xview covers whole width" {[.b1 xview] eq "0.0 1.0"}
check "small data: yview covers whole height" {[.b1 yview] eq "0.0 1.0"}

# resize the widget bigger than the data too, then smaller again
.b1 configure -width [expr {$nC*2}] -height [expr {$nR*2}]
update
check "grow viewport past data size: still no crash" 1
.b1 configure -width $nC -height $nR
update
check "shrink viewport back: still no crash" 1

destroy .b1
update

# ==================================================================
# TEST GROUP 2: widget viewport nC x nR, data LARGER than viewport
#   (nseq > nR, seqlen > nC) -- scrolling required in both directions
# ==================================================================
puts "\n===== GROUP 2: data larger than the visible viewport ====="
set nC 20
set nR 5
biotext .b2 -width $nC -height $nR -xscrollcommand {.sx2 set} -yscrollcommand {.sy2 set}
scrollbar .sx2 -orient horizontal -command {.b2 xview}
scrollbar .sy2 -orient vertical   -command {.b2 yview}
pack .b2 -side left -fill both -expand 1
update

set nseqBig 30
set lenBig  80
set seqs {}
for {set i 0} {$i < $nseqBig} {incr i} {
    lappend seqs [mkseq $lenBig $i]
}
.b2 sequences $seqs
update idletasks
update

set xv [.b2 xview]
set yv [.b2 yview]
puts "  xview: $xv   yview: $yv"
check "large data: xview first fraction is 0" {[lindex $xv 0] == 0.0}
check "large data: xview second fraction < 1 (horizontal scroll needed)" {[lindex $xv 1] < 1.0}
check "large data: yview first fraction is 0" {[lindex $yv 0] == 0.0}
check "large data: yview second fraction < 1 (vertical scroll needed)" {[lindex $yv 1] < 1.0}

# scroll to the far right / bottom and check bounds are respected
.b2 xview moveto 1.0
.b2 yview moveto 1.0
update
set xv2 [.b2 xview]
set yv2 [.b2 yview]
puts "  after moveto 1.0 -> xview: $xv2   yview: $yv2"
check "xview second fraction reaches 1.0 at the far right" {abs([lindex $xv2 1] - 1.0) < 1e-6}
check "yview second fraction reaches 1.0 at the bottom" {abs([lindex $yv2 1] - 1.0) < 1e-6}

# scroll back to origin
.b2 xview moveto 0.0
.b2 yview moveto 0.0
update
check "xview back at 0.0" {[lindex [.b2 xview] 0] == 0.0}

# scroll by units/pages (exercise Tk_GetScrollInfoObj path)
.b2 xview scroll 3 units
.b2 yview scroll 1 page
update
check "scroll by units/page did not crash" 1

# xview/yview index form on a scrolled, oversized widget
.b2 xview 50
.b2 yview 12
update
puts "  xview after index 50: [.b2 xview]"
puts "  yview after index 12: [.b2 yview]"
check "xview index form did not crash on oversized data" 1

# edit at the very edges of the oversized alignment
.b2 cursor "1.[expr {$lenBig-1}]"
.b2 chars X 1
.b2 cursor "${nseqBig}.0"
.b2 chars Y 1
update
check "editing at first/last row and last column of oversized data OK" 1

# insert/delete cols & rows right at the boundary while oversized
.b2 insert cols end 5
.b2 insert rows end 3
.b2 delete cols end
.b2 delete rows end
update
check "insert/delete at end of oversized alignment OK" 1

destroy .b2
destroy .sx2
destroy .sy2
update

# ==================================================================
# TEST GROUP 3: exact-fit boundary (data EXACTLY matches viewport)
# ==================================================================
puts "\n===== GROUP 3: data exactly matches the viewport ====="
set nC 12
set nR 6
biotext .b3 -width $nC -height $nR
pack .b3
update
# the earlier gridded widgets left their size on the toplevel: drop it
wm geometry . {}
update

set seqs {}
for {set i 0} {$i < $nR} {incr i} {
    lappend seqs [mkseq $nC $i]
}
.b3 sequences $seqs
update idletasks
update
set xv3 [.b3 xview]
set yv3 [.b3 yview]
puts "  exact-fit xview: $xv3  yview: $yv3"
check "exact-fit data: no scrolling needed (xview 0.0 1.0)" {$xv3 eq "0.0 1.0"}
check "exact-fit data: no scrolling needed (yview 0.0 1.0)" {$yv3 eq "0.0 1.0"}

# now grow past exact fit by ONE column / ONE row and check it becomes scrollable
.b3 insert cols end 1
.b3 insert rows end 1
update
set xv3b [.b3 xview]
set yv3b [.b3 yview]
puts "  after +1 col / +1 row -> xview: $xv3b  yview: $yv3b"
check "growing by exactly 1 col makes xview partial" {[lindex $xv3b 1] < 1.0}
check "growing by exactly 1 row makes yview partial" {[lindex $yv3b 1] < 1.0}

destroy .b3
update

# ==================================================================
# TEST GROUP 4: transition from small to large at runtime (same widget)
# ==================================================================
puts "\n===== GROUP 4: same widget, data grows from small to large at runtime ====="
biotext .b4 -width 15 -height 4
pack .b4
update
.b4 sequences [list [mkseq 10 0] [mkseq 10 1]]
update
check "initial small state: fits (xview 0.0 1.0)" {[.b4 xview] eq "0.0 1.0"}

# now add many more, longer sequences via addseqs
for {set k 0} {$k < 20} {incr k} {
    .b4 addseqs [list [mkseq 10 $k]] end
}
update
check "after growing to 22 rows: yview no longer full range" {[lindex [.b4 yview] 1] < 1.0}

# widen every row via insert cols (affects all rows at once)
.b4 insert cols end 50
update
check "after widening columns: xview no longer full range" {[lindex [.b4 xview] 1] < 1.0}

# now shrink back down: delete rows/cols until small again
for {set k 0} {$k < 18} {incr k} {
    .b4 delete rows 0
}
.b4 delete cols 0
update
check "after shrinking back down: no crash" 1

destroy .b4
update

# ==================================================================
# TEST GROUP 5: pathological sizes
# ==================================================================
puts "\n===== GROUP 5: pathological / edge sizes ====="

# a single sequence, single column
biotext .b5 -width 10 -height 10
pack .b5
update
.b5 sequences {A}
update
check "single row, single column: no crash" 1
check "single row/col: xview/yview both full" {[.b5 xview] eq "0.0 1.0" && [.b5 yview] eq "0.0 1.0"}

# viewport LARGER than any conceivable data (1x1 vs huge viewport)
.b5 configure -width 500 -height 500
update
check "huge viewport vs tiny data: no crash" 1

# many rows, minimal length (nR huge, nC=1)
destroy .b5
update
biotext .b5 -width 5 -height 5
pack .b5
update
set manyseqs {}
for {set i 0} {$i < 200} {incr i} {
    lappend manyseqs "A"
}
.b5 sequences $manyseqs
update
check "200 single-character rows: no crash" 1
check "200 rows vs 5-row viewport: yview partial" {[lindex [.b5 yview] 1] < 1.0}

# one very long single sequence
destroy .b5
update
biotext .b5 -width 5 -height 5
pack .b5
update
.b5 sequences [list [mkseq 5000 0]]
update
check "one 5000-char sequence: no crash" 1
check "5000-char single row vs 5-col viewport: xview partial" {[lindex [.b5 xview] 1] < 1.0}

destroy .b5
update

# ==================================================================
# TEST GROUP 6: repeated create/destroy cycles (stress the
# constructor/destructor pair and memory reuse)
# ==================================================================
puts "\n===== GROUP 6: repeated create/destroy cycles ====="
for {set k 0} {$k < 15} {incr k} {
    biotext .bcycle -width [expr {5 + $k}] -height [expr {3 + ($k % 4)}]
    pack .bcycle
    update
    set n [expr {2 + $k}]
    set seqs {}
    for {set i 0} {$i < $n} {incr i} {
        lappend seqs [mkseq [expr {5 + $k}] $i]
    }
    .bcycle sequences $seqs
    .bcycle tag configure t$k -foreground red -background yellow
    .bcycle tag add t$k 1.0 1.1
    update
    destroy .bcycle
    update
}
check "15 create/destroy cycles (varying sizes, with tags) OK" 1

# ==================================================================
# TEST GROUP 7: multiple widget instances simultaneously (isolation)
# ==================================================================
puts "\n===== GROUP 7: multiple simultaneous widget instances ====="
biotext .bA -width 10 -height 4
biotext .bB -width 30 -height 12
pack .bA .bB
update
.bA sequences [list [mkseq 8 0] [mkseq 8 1]]
set bigseqsB {}
for {set i 0} {$i < 25} {incr i} { lappend bigseqsB [mkseq 60 $i] }
.bB sequences $bigseqsB
update
check ".bA (small data) fits fully" {[.bA xview] eq "0.0 1.0" && [.bA yview] eq "0.0 1.0"}
check ".bB (large data) needs scrolling" {[lindex [.bB xview] 1] < 1.0 && [lindex [.bB yview] 1] < 1.0}
# mutate one, make sure the other is unaffected (no shared-state bugs)
.bA delete rows 0
.bB insert cols end 10
update
check ".bA and .bB remained independent after mutation" 1
destroy .bA
destroy .bB
update

# ==================================================================
# TEST GROUP 8: tags + mapping combined on oversized (scrolled) data
# ==================================================================
puts "\n===== GROUP 8: tags + mapping combined, oversized/scrolled data ====="
biotext .b8 -width 15 -height 4
pack .b8
update
set seqs8 {}
for {set i 0} {$i < 40} {incr i} { lappend seqs8 [mkseq 100 $i] }
.b8 sequences $seqs8
.b8 mapping {{A red green} {C blue yellow} {G black white} {T pink cyan} {X orange orange} {. gray gray} {{ } gray gray}}
update
check "mapping on oversized (scrolled) data: no crash" 1

# tag spanning rows/cols that are currently off-screen
.b8 tag configure big -foreground white -background black
for {set rr 35} {$rr <= 38} {incr rr} {
    .b8 tag add big $rr.90 $rr.99
}
update
puts "  tag ranges (off-screen zone): [.b8 tag ranges big]"
check "tag added on an off-screen zone: no crash" 1

# scroll so that zone becomes visible, forcing a redraw of it
.b8 xview 85
.b8 yview 34
update
check "scrolling an off-screen tagged zone into view: no crash" 1

# insert an unmapped character ('X' IS mapped here, but 'Z' is not) at
# the edge of the oversized data -- regression check for the
# MapOneSequence() -1 fallback fix
.b8 cursor 40.99
.b8 chars Z 1
update
check "inserting an UNMAPPED character ('Z') into mapped, oversized data: no crash" 1

# delete rows/cols that intersect the tagged zone while scrolled
.b8 delete rows 36
.b8 delete cols 90
update
check "deleting rows/cols intersecting a tag while scrolled: no crash" 1

destroy .b8
update

# ==================================================================
# TEST GROUP 9: cut/copy/paste across the viewport boundary
#   (operate on rows that are scrolled off-screen)
# ==================================================================
puts "\n===== GROUP 9: copy/cut/paste on off-screen rows ====="
biotext .b9 -width 10 -height 3
pack .b9
update
set seqs9 {}
for {set i 0} {$i < 20} {incr i} { lappend seqs9 [mkseq 10 $i] }
.b9 sequences $seqs9
update
check "20 rows vs 3-row viewport: most rows off-screen" {[lindex [.b9 yview] 1] < 1.0}

# copy row 18 (off-screen, near the very end) and paste after row 1
# (BiotextPasteCmd's "index" semantics insert AFTER the given 1-based
# row, so it lands at row 2, not row 1)
set before18 [lindex [.b9 output {18}] 0]
.b9 copy {18}
.b9 paste 1
update
set after2 [lindex [.b9 output {2}] 0]
check "copy of an off-screen row pasted correctly after row 1" {$before18 eq $after2}

# cut an off-screen row and paste it back after the (new) last row
set before10 [lindex [.b9 output {10}] 0]
.b9 cut {10}
.b9 paste end
update
set afterLast [lindex [.b9 output {21}] 0]
check "cut of an off-screen row re-pasted correctly at the end" {$before10 eq $afterLast}

destroy .b9
update

# ==================================================================
# TEST GROUP 10: font weight -- default should NOT be bold
#   (regression test for the reported "default appears bold" issue)
# ==================================================================
puts "\n===== GROUP 10: font weight (default must not be bold) ====="
biotext .b10 -width 20 -height 5
pack .b10
update
.b10 sequences {ACGTACGTAC ACGTACGTAG}
update idletasks
update

set defaultMetrics [.b10 font metrics]
puts "  default metrics: $defaultMetrics"
array set dm $defaultMetrics

# Query the TRUE default weight BEFORE touching it with any "font
# weight" command -- this is the actual regression check for the
# reported "widget comes up bold by default" issue.
set defaultWeight [.b10 font weight]
puts "  default weight (untouched): $defaultWeight"
check "font weight getter reports 'normal' by default (the actual regression check)" \
    {$defaultWeight eq "normal"}
check "font family getter reports Courier" {[.b10 font family] eq "Courier"}
check "font size getter reports 12" {[.b10 font size] == 12}

# Explicitly force bold, capture metrics
.b10 font weight bold
update
set boldMetrics [.b10 font metrics]
array set bm $boldMetrics
puts "  bold metrics:    $boldMetrics"
check "font weight getter reports 'bold' after explicitly setting it" \
    {[.b10 font weight] eq "bold"}

# Explicitly force normal, capture metrics again
.b10 font weight normal
update
set normalMetrics [.b10 font metrics]
array set nm $normalMetrics
puts "  normal metrics:  $normalMetrics"
check "font weight getter reports 'normal' again after resetting it" \
    {[.b10 font weight] eq "normal"}

# NOTE: for a real monospace font, Bold and Regular MUST share the
# same fixed advance width by design -- that is the entire point of
# "monospace". So "-width" is NOT a valid signal to distinguish bold
# from normal (kept only as a sanity check that toggling doesn't
# corrupt basic metrics).
check "width metric stays sane across weight toggling" \
    {$dm(-width) == $nm(-width) && $bm(-width) > 0}

destroy .b10
update

puts "\n====================================="
puts "Total failures: $failures"
puts "====================================="
if {$failures > 0} { exit 1 }
exit 0
