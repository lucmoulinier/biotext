#!/usr/local/bin/wish



#package require biotext
#source /home/moumou/ordali/src/ordali_sequence.tcl

#oload Windows/biotext0.1.dll
#load biotext1.0.dll
load ./libbiotext1.0.so
#load libbiotext1.0.dylib
#package require biotext
puts "livrary  loaded"

package require platform
if {[string range [platform::generic] 0 4] eq "win32"} {
    set OrdaliDir [file join o:/ ordali]
    set GscopeDir [file join m:/ ripp gscope]
} elseif {[string range [platform::generic] 0 4] eq "macos"} {
    set OrdaliDir /Users/Luc/moumou/ordali/
    set GscopeDir /Users/Luc/moumou/ordali/src
} else {
    set OrdaliDir $env(ORDALIDIR)
    set GscopeDir /home/ripp/gscope
}
puts "ordalidir : $OrdaliDir"
source [file join $OrdaliDir src ordali_source.tcl]
source [file join $GscopeDir gscope_outils.tcl]
puts "sources done"

LesDefauts

proc ChgFontSize {incr} {
    set size [font configure Luc -size]
    puts "\nfont avnt $size"
    
    set size [expr {$size + $incr}]
    if {$size > 5 && $size < 42} {
	font configure Luc -size $size
	puts "font apres $size"
	update idletasks
    }
    
    return
}


proc BindScroll {} {
    global Defauts

    bind $::WTxt <4> {CoupleY scroll -[set Defauts(DeltaYScroll)] units ; break}
    bind $::WTxt <5> {CoupleY scroll [set Defauts(DeltaYScroll)] units ; break}

    bind $::WTxt <Control-4> [list $::WTxt xview scroll -[set Defauts(DeltaYScroll)] units]
    bind $::WTxt <Control-5> [list $::WTxt xview scroll [set Defauts(DeltaYScroll)] units]

    return
}


proc ToggleWdg {what} {
    global WTxt
    
    if {$what eq $WTxt} {
	return
    }
    
    lassign [.f.sv get] yscr tmp
    
    set wold $WTxt
    set WTxt $what
    if {0 && $WTxt eq ".f.bs"} {
	set wdt [$wold cget -width]
	set hgt [$wold cget -height]
	.f.bs configure \
	    -width $wdt \
	    -height $hgt
    }
    grid forget $wold
    grid $WTxt -row 0 -column 1 -sticky news

    BindScroll
    CoupleY moveto $yscr
    
    update idletasks
    
    puts "WTxt $WTxt"
    
    return
}


proc CoupleY {args} {
    #puts "CoupeY : $args"
    $::WTxt yview {*}$args
    .f.tn   yview {*}$args

    return
}

##########################
font create Luc -family Courier -size 8

grid columnconfig . 0 -weight 1
grid rowconfig    . 0 -weight 1

frame .f \
    -background black
grid .f -row 0 -column 0 -sticky news
grid columnconfig .f 1 -weight 1

set WTxt .f.ts

text .f.tn \
    -foreground white \
    -background black \
    -width 9 \
    -height 10 \
    -font Luc \
    -wrap none \
    -xscrollcommand ".f.sh set " \
    -yscrollcommand ".f.sv set"
text .f.ts \
    -foreground white \
    -background black \
    -height 10 \
    -width 15 \
    -font Luc \
    -wrap none \
    -xscrollcommand ".f.sh set " \
    -yscrollcommand ".f.sv set"
text .f.t2 \
    -height 10 \
    -font Luc \
    -background black \
    -foreground yellow \
    -wrap none \
    -xscrollcommand ".f.sh set " \
    -yscrollcommand ".f.sv set"
biotext .f.bs \
    -foreground white \
    -background black \
    -bd 1 \
    -xscrollcommand ".f.sh set " \
    -yscrollcommand ".f.sv set" \
    -width 15 \
    -height 10 \
    -class "Biotext" \
    -font Luc
#.f.bs font family Courier size 24 weight normal
scrollbar .f.sh \
    -width 20 \
    -troughcolor magenta \
    -bg blue \
    -orient horiz \
    -width 20 \
    -command [list $WTxt xview ]
scrollbar .f.sv \
    -width 20 \
    -troughcolor magenta \
    -bg blue \
    -command "CoupleY "
frame .fb \
    -bg black
grid .fb -row 2 -column 0 -columnspan 3 -sticky ew
button .fb.b1 \
    -text " Biotext " \
    -font Luc \
    -bg blue \
    -fg yellow \
    -command {ToggleWdg .f.bs}
button .fb.b2 \
    -text " Text 2 " \
    -font Luc \
    -bg green \
    -fg white \
    -command {ToggleWdg .f.t2}
button .fb.b3 \
    -text " Ori Text " \
    -font Luc \
    -bg black \
    -fg white \
    -command {ToggleWdg .f.ts}
button .fb.b4 \
    -text " Ori Text " \
    -font Luc \
    -bg black \
    -fg white \
    -command {ToggleWdg .f.ts}
button .fb.b5 \
    -text " Ori Text " \
    -font Luc \
    -bg black \
    -fg white \
    -command {ToggleWdg .f.ts}
button .fb.b6 \
    -text " Ori Text " \
    -font Luc \
    -bg black \
    -fg white \
    -command {ToggleWdg .f.ts}

grid .fb.b1 -row 0 -column 0 -sticky e -padx 20
grid .fb.b2 -row 0 -column 1 -sticky e -padx 20
grid .fb.b3 -row 0 -column 2 -sticky e -padx 20
#grid .fb.b4 -row 0 -column 3 -sticky e -padx 20
#grid .fb.b5 -row 0 -column 4 -sticky e -padx 20
#grid .fb.b6 -row 0 -column 5 -sticky e -padx 20

grid .f.tn -row 0 -column 0 -sticky ns
grid .f.ts -row 0 -column 1 -sticky news
grid .f.sv -row 0 -column 2 -sticky ns
grid .f.sh -row 1 -column 1 -sticky ew
grid columnconfig .f 1 -weight 1

grid rowconfig    .f 0 -weight 1

.f.tn configure -state disabled
.f.ts configure -state disabled
.f.bs configure -state disabled


bind .f <Shift-Key-e> ToggleWdg

bind .f <Control-Key-p> {ChgFontSize 2 ; break}
bind .f <Control-Key-m> {ChgFontSize -2 ; break}

bind .f <Control-Key-p> {ChgFontSize 2}
bind .f <Control-Key-m> {ChgFontSize -2}

bind .f.tn <4> {CoupleY scroll -[set Defauts(DeltaYScroll)] units ; break}
bind .f.tn <5> {CoupleY scroll [set Defauts(DeltaYScroll)] units ; break}

BindScroll

set tfa $argv
LitLeTFA $tfa Lnom Tseq
set Lseq [lmap n $Lnom {set Tseq($n)}]

.f.ts configure -state normal
.f.ts insert end [join $Lseq \n]
.f.t2 insert end [join $Lseq \n]

.f.bs sequences $Lseq
.f.bs mapping [set Defauts(Mapping)]
.f.bs map on
.f.bs configure -state normal

.f.tn configure -state normal
.f.tn insert end "[join $Lnom \n]"
.f.tn configure -state disabled

proc BindSpace {w} {
    set idx [$w cursor]
    lassign [split $idx .] y x
    $w chars "." $idx 1
}

#bind .f.bs <Control-Key-c> {%W delete colsgaps}
#bind .f.b <Key-space> {BindSpace %W}

FenetreAuCentre .

