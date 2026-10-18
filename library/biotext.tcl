# biotext.tcl --
#
# This file defines the default bindings for Tk 
# biotext widgets class and provides
# procedures that help in implementing those bindings.
#
# Copyright (c) 2013 Luc Moulinier and LBGI,
# (Laboratoire de Biologie et Genomique Integratines)
#


#-------------------------------------------------------------------------
# Elements of tk::Priv that are used in this file:
#
# nInBuffer - 
# records numeric keys that will, when
# invoked, give how many times a given
# command should be done.
#
# mouseMoved - 
# Non-zero means the mouse has moved a significant
# amount since the button went down (so, for example,
# start dragging out a selection).
#
# pressX - 
# X-coordinate at which the mouse button was pressed.
#
# selectMode -
# The style of selection currently underway:
# char, word, or line.
# x, y -
# Last known mouse coordinates for scanning
# and auto-scanning.
#
# data -
# Used for Cut and Copy
#-------------------------------------------------------------------------
#
# Initialisation -
#

namespace eval ::tk::biotext {
    # Ensure that a namespace is created for us
    variable Priv

    set Priv(x) 0
    set Priv(y) 0
    set Priv(nInBuffer) ""
    set Priv(mouseMoved) 0
    
    namespace export cursor
}

# Bindings of numbers.
package require platform

# numbers attached to the left keypad keys
set ckp [list KP_Insert KP_End KP_Down KP_Next KP_Left KP_Begin KP_Right KP_Home KP_Up KP_Prior]
set Lc [list 0 1 2 3 4 5 6 7 8 9]
foreach c $Lc k $ckp {
    bind Biotext "<Key-$c>"    {::tk::biotext::bufferNTimes "add" %A}
    bind Biotext "<Shift-Key-$c>"    {::tk::biotext::bufferNTimes "add" %A}
    bind Biotext "<Key-KP_$c>" {::tk::biotext::bufferNTimes "add" %A}
    if {$::tcl_platform(platform) ne "windows" && ! [regexp {^macos} [::platform::generic]] } {
	bind Biotext <Key-$k>  {::tk::biotext::bufferNTimes "add" %A}
    }
}


# bind mouse buttons
bind Biotext <1> {
    tk::biotext::button1 %W %x %y
}

bind Biotext <Shift-Left> {
    focus %W
    %W push left ; update idletasks ; break
}

bind Biotext <Shift-Right> {
    focus %W
    %W push right ; update idletasks ; break
}

#bind Biotext <Left>  {tk::biotext::cursor %W -1  0}
#bind Biotext <Right> {tk::biotext::cursor %W  1  0}
#bind Biotext <Up>    {tk::biotext::cursor %W  0 -1}
#bind Biotext <Down>  {tk::biotext::cursor %W  0  1}

bind Biotext <BackSpace> {
    focus %W
    %W delchars [%W cursor] [::tk::biotext::bufferNTimes "empty"]
}

bind Biotext <Shift-BackSpace> {
    focus %W
    %W delchars -group [%W cursor] [::tk::biotext::bufferNTimes "empty"]
}


#-------------------------------------------------------------------------
# The code below creates the default class  
# bindings for biotext widgets.
#-------------------------------------------------------------------------


# ::tk::biotext::cursor --
# This procedure is invoked to set the cursor place in
# biotext widgets.
# It moves the cursor from the "current" position to
# the desired place taking into
# account the N Times buffer, which default is 1 time.
#
# Arguments:
# w - The biotext window in which to set the cursor
# x - The increment in rows
# y - The increment in columns
proc ::tk::biotext::cursor {w x y} {
    return
    if {[$w cget -state] eq "disabled"} {
	return
    }

    lassign [split [$w cursor] .] r c
    set n [::tk::biotext::bufferNTimes "empty"]
    puts "\n::cursor x=$x y=$y\nIn  r=$r c=$c\nOut y=[expr {$r + $y*$n}] c=[expr {$c + $x*$n}]"
    $w cursor [incr r [expr {$y*$n}]].[incr c [expr {$x*$n}]]
    $w see $r.$c
    update idletasks

    return
}

# ::tk::biotext::button1 --
# This procedure is invoked to handle button-1 
# presses in biotext widgets.
# It moves the insertion cursor and claims the input 
# focus.
#
# Arguments:
# w - The biotext window in which the button was 
# pressed.
# x - The x-coordinate (pixels) of the button press.
# y - The y-coordinate (pixels) of the button press.

proc ::tk::biotext::button1 {w x y} {
    array set ::tk::biotext::Priv [list selectMode char mouseMoved 0 pressX $x pressY $y]
    $w cursor @$x,$y
    if {"disabled" ne [$w cget -state]} {
	focus $w
    }

    return
}

proc ::tk::biotext::bufferNTimes {what {n ""}} {
    set res 1
    switch $what {
	"add" {
	    append ::tk::biotext::Priv(nInBuffer) $n
	}
	"empty" {
	    if {$::tk::biotext::Priv(nInBuffer) ne ""} {
		set res $::tk::biotext::Priv(nInBuffer)
		set ::tk::biotext::Priv(nInBuffer) ""
	    }
	}
    }

    return $res
}


