#!/usr/local/bin/wish



proc CreateData {{num ""} {len ""}} {
    if {$num eq ""} {set num 50}
    if {$len eq ""} {set len 300}

    set Lnoms {}
    for {set i 1} {$i <= $num} {incr i} {
	lappend Lnoms "seq_[format %02d $i]"
    }

    set tank "ACDEFGHIKLMNPQRSTVWY...................."
    set nDigits [string length $tank]
    set Lseqs {}
    for {set i 0} {$i < $num} {incr i} {
	set seq ""
	for {set j 0} {$j < $len} {incr j} {
	    set x [expr {int(rand()*$nDigits)}]
	    if {$x == $nDigits} {
		set x [expr {$nDigits -1}]
	    }
	    append seq [string index $tank $x]
	}
	lappend Lseqs $seq
    }
    
    return [list $Lnoms $Lseqs]
}

proc CoupleY {args} {
    .f.noms yview {*}$args
    .f.seqs yview {*}$args

    return
}


set auto_path [linsert $auto_path 0 [file normalize [file join [pwd] ..]]]
lappend auto_path /home/moumou/ordali2/lib
#package require biotext 1.0
load ./libbiotext1.0.so
source ./library/biotext.tcl

wm title . "Example 2"
. configure -background black
grid columnconfig . 0 -weight 1
grid rowconfig . 0 -weight 1

frame .f
grid .f -row 0 -column 0 -sticky news
grid columnconfig .f 1 -weight 1
grid rowconfig .f 0 -weight 1
set Bnom .f.noms
biotext .f.noms \
    -width 10 \
    -height 20 \
-background white \
    -foreground black \
    -bd 1 \
    -yscrollcommand ".f.vs set "

set Bseqs .f.seqs
biotext .f.seqs \
    -width 100 \
    -height 20 \
-background black \
    -foreground white \
    -bd 1 \
    -yscrollcommand ".f.vs set " \
    -xscrollcommand ".f.hs set "

scrollbar .f.vs \
    -width 8 \
    -orient vertical \
    -command [list CoupleY]
scrollbar .f.hs \
    -width 8 \
    -orient horizontal \
    -command ".f.seqs xview "

grid .f.noms -row 0 -column 0 -sticky ns
grid .f.seqs -row 0 -column 1 -sticky news
grid .f.hs  -row 1 -column 1 -sticky ew
grid .f.vs  -row 0 -column 3 -sticky ns

lassign [CreateData] Lnoms Lseqs
.f.noms sequences $Lnoms
.f.seqs sequences $Lseqs




