# "tag add" : plages conservées, mémoire stable (Ordalie audit, B4),
# index invalide refusé sans modifier le tag.
load ./libtkbiotext.so Biotext
proc R {} {set f [open /proc/self/statm]; lassign [read $f] a b; close $f; expr {$b*4/1024}}
set seq [string repeat ACDEFGHIKL 160]
biotext .b -width 40 -height 10
pack .b
set Ls {}
for {set i 0} {$i < 165} {incr i} {lappend Ls $seq}
.b sequences $Ls
set Lr {}
for {set r 1} {$r <= 165} {incr r} {for {set c 0} {$c < 1600} {incr c 2} {lappend Lr $r.$c $r.$c}}
set echecs 0
proc V {tag ok {info ""}} {puts "[expr {$ok ? {OK  } : {FAIL}}] $tag $info"; if {! $ok} {incr ::echecs}}
.b configure -state normal
.b tag configure ZZ -foreground black -background red
.b tag add ZZ 2.3 2.5 2.10 2.11
.b tag add ZZ 2.20 2.21
.b tag add ZZ 1.7 1.8
set r [.b tag ranges ZZ]
V "ajouts successifs conservés" [expr {[llength $r] == 8}] $r
V "index invalide refusé" [catch {.b tag add ZZ 3.1 3.2 pas_un_index 3.4}]
V "index invalide : tag intact" [expr {[.b tag ranges ZZ] eq $r}]
.b tag delete ZZ
set t0 [clock milliseconds]
.b tag configure ZZ -foreground black -background red
.b tag add ZZ {*}$Lr
set ms [expr {[clock milliseconds]-$t0}]
V "grand ajout conservé" [expr {[llength [.b tag ranges ZZ]] == [llength $Lr]}] "[llength $Lr] / $ms ms"
.b tag delete ZZ
update
set r0 [R]
for {set i 0} {$i < 8} {incr i} {
    .b tag configure ZZ -foreground black -background red
    .b tag add ZZ {*}$Lr
    update
    .b tag delete ZZ
}
set r1 [R]
V "mémoire stable sur 8 cycles" [expr {$r1-$r0 < 20}] "$r0 -> $r1 Mo"
puts "BILAN $echecs"
exit $echecs
