# biotext : commandes appelées avec des index hors limites ou des arguments incohérents. Aucune ne doit faire
# planter wish (erreur Tcl acceptée). Chaque cas tourne dans un wish à part (voir test_index_hors_limites.sh).
# usage : wish test_index_hors_limites.tcl <bibliothèque> <numéro du cas>   ; sans numéro : liste des cas
lassign $argv lib cas
set Cas {
    {addseqs {.b addseqs {..........} 5}}
    {addseqs {.b addseqs {..........} 2}}
    {addseqs {.b addseqs {..........} -2}}
    {addseqs {.b addseqs {..........} 99}}
    {addseqs {.b addseqs {.... ..........................} 0}}
    {addseqs {.b addseqs {..........} -1}}
    {addseqs {.b addseqs {..........} end}}
    {chars {.b chars X 9.0}}
    {chars {.b chars X 1.99}}
    {delchars {.b delchars 9.3 1}}
    {delchars {.b delchars 1.99 3}}
    {cursor {.b cursor 9.0}}
    {cursor {.b cursor -1.-1}}
    {delete {.b delete rows 9}}
    {delete {.b delete rows -1}}
    {delete {.b delete cols 99 1}}
    {insert {.b insert rows 9 1}}
    {insert {.b insert cols 99 1}}
    {tag {.b tag configure T -foreground black -background red; .b tag add T 9.0 9.3}}
    {tag {.b tag configure T -foreground black -background red; .b tag add T 1.5 1.99}}
    {tag {.b tag configure T -foreground black -background red; .b tag add T 1.0 1.3; .b tag remove T 9.0 9.3}}
    {tag {.b tag configure T -foreground black -background red; .b tag add T -1.0 0.3}}
    {see {.b see 99.99}}
    {output {.b output 99}}
    {push {.b cursor 1.3; .b push right}}
    {copy {.b copy 9.0 9.3}}
    {paste {.b paste 9.0}}
    {group {.b group 9}}
}
if {$cas eq ""} {puts [llength $Cas]; exit}
load $lib Biotext
biotext .b -width 20 -height 4
pack .b; update
.b sequences {AAAAACCCCC GGGGGTTTTT}
update
lassign [lindex $Cas $cas] nom script
set r [catch {uplevel #0 $script} m]
update
# le widget doit encore répondre
set ok [expr {! [catch {.b output all}]}]
puts "$cas $nom : [expr {$r ? "erreur Tcl ([string range $m 0 60])" : "accepté"}] ; widget [expr {$ok ? "valide" : "INVALIDE"}]"
exit
