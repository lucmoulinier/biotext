#!/bin/bash
# lance chaque cas de test_index_hors_limites.tcl dans un wish à part ; échec si un cas fait planter wish
# usage : test_index_hors_limites.sh <bibliothèque> [wish]
LIB=$(readlink -f "$1"); W=${2:-wish8.6}; D=$(dirname "$(readlink -f "$0")")
n=$($W $D/test_index_hors_limites.tcl "$LIB" 2>/dev/null); rc=0
for ((i=0; i<n; i++)); do
  out=$(timeout 20 $W $D/test_index_hors_limites.tcl "$LIB" $i 2>&1); c=$?
  if [ $c -ne 0 ]; then echo "[FAIL] cas $i : wish terminé (code $c)"; rc=1; else echo "[ OK ] $out"; fi
done
exit $rc
