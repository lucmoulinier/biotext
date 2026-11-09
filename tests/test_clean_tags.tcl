# biotext : "clean" must empty the order of the tags (bugs.txt, 70)
# usage : wish test_clean_tags.tcl <dir containing biotext1.0>
lassign $argv dir
lappend auto_path $dir
package require biotext
biotext .b -width 20 -height 4
pack .b; update
.b sequences {AAAAACCCCC GGGGGTTTTT}
foreach t {Fnd Hel She} {.b tag configure $t -foreground black -background red; .b tag add $t 1.0 1.3}
.b clean
.b sequences {AAAAACCCCC GGGGGTTTTT}
foreach t {Fnd Hel She Anno_1 Anno_2} {.b tag configure $t -foreground black -background blue; .b tag add $t 2.1 2.4}
set n [.b tag names]
puts "apres clean : $n"
puts [expr {$n eq {Fnd Hel She Anno_1 Anno_2} ? {[ OK ] tag names apres clean} : {[FAIL] tag names apres clean}}]
.b tag delete Anno_1
set n [.b tag names]
puts [expr {$n eq {Fnd Hel She Anno_2} ? {[ OK ] tag delete apres clean} : "\[FAIL\] tag delete apres clean : $n"}]
destroy .b
exit 0
