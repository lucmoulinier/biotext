load ./libtkbiotext.so Biotext

puts "=== Package loaded OK ==="

# 1. Widget creation
biotext .b -width 40 -height 10 -xscrollcommand {.sx set} -yscrollcommand {.sy set}
scrollbar .sx -orient horizontal -command {.b xview}
scrollbar .sy -orient vertical   -command {.b yview}
pack .b -side left -fill both -expand 1
puts "=== Widget created OK ==="

# 2. Configure
.b configure -background white -foreground black
.b configure -font {Courier 12}
puts "config font/colors: [.b cget -font] / [.b cget -background]"

# 3. Set initial sequences (must use "sequences" to populate an
#    initially-empty widget -- "addseqs" requires nbSeqs>0 already)
.b sequences {ACGTACGTAC ACGTACGTAG ACGTTCGTAC ACGTACGTAA}
update
puts "=== sequences (initial population) + update OK ==="

.b addseqs {ACGTACGTAA} end
update
puts "=== addseqs + update (forces BiotextDisplay) OK ==="

# 4. Force a draw / geometry pass
update idletasks
.b configure -width 20 -height 3
update
puts "=== resize + redraw OK ==="

# 5. Character editing
.b cursor 1.2
.b chars a 3
puts "=== chars insert OK ==="
.b delchars 1.2
puts "=== delchars OK ==="

# 6. Insert/delete cols and rows (edge cases)
.b insert cols 0 -2
.b insert cols end 3
.b delete cols 0
.b delete cols end
puts "=== insert/delete cols edge cases OK ==="

.b insert rows 0 -1
.b insert rows end 2
.b delete rows 0
.b delete rows end
puts "=== insert/delete rows edge cases OK ==="

# 7. Tags
.b tag configure mytag -foreground red -background yellow
.b tag add mytag 1.0 1.3
.b tag add mytag 2.1 2.4
puts "tag ranges mytag: [.b tag ranges mytag]"
.b tag remove mytag 1.0 1.1
.b tag names 1.0
puts "=== tags add/remove/ranges/names OK ==="
.b tag raise mytag
.b tag state on
.b tag delete mytag
puts "=== tag raise/state/delete OK ==="

# 8. Mapping
.b mapping {{A red green} {C blue yellow} {G black white} {T pink cyan} {. gray gray} {{ } gray gray}}
update
puts "=== mapping configured + redrawn OK ==="
.b map on
update
.b map clean
update
.b map off
puts "=== map on/clean/off OK ==="

# 9. Groups
.b group -new 1 {0 1}
.b group -merge 1 {2}
puts "grouped."
.b ungroup {0 1 2}
puts "=== group/ungroup (incl. previously-empty -merge) OK ==="

# 10. Copy/cut/paste (regression check for earlier fixes)
.b copy {1}
.b paste end
.b cut {2}
.b paste 1
update
puts "=== copy/cut/paste OK ==="

# 11. Cursor movement / push / see
.b cursor 1.1
.b push right
.b see end
puts "=== cursor/push/see OK ==="

# 12. Scroll xview/yview (index form -- was the empty TO DO stub)
.b xview 2
.b yview 1
puts "xview after set: [.b xview]"
puts "yview after set: [.b yview]"
puts "=== xview/yview index form OK ==="

# 13. destroy widget (exercise BiotextDeletedProc cleanup path)
destroy .b
update
puts "=== widget destroyed cleanly OK ==="

puts "\n=== ALL SMOKE TESTS PASSED ==="
exit 0
