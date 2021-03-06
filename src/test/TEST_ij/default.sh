#!/bin/sh
# Copyright 1998-2019 Lawrence Livermore National Security, LLC and other
# HYPRE Project Developers. See the top-level COPYRIGHT file for details.
#
# SPDX-License-Identifier: (Apache-2.0 OR MIT)

TNAME=`basename $0 .sh`
RTOL=$1
ATOL=$2

#=============================================================================
# IJ: Run 2 and 3 proc parallel case, weighted Jacobi, BoomerAMG 
#                    diffs it against 1 proc case
#=============================================================================

tail -17 ${TNAME}.out.0 | head -6 > ${TNAME}.testdata

#=============================================================================

tail -17 ${TNAME}.out.1 | head -6 > ${TNAME}.testdata.temp
diff ${TNAME}.testdata ${TNAME}.testdata.temp >&2

#=============================================================================

tail -17 ${TNAME}.out.2 | head -6 > ${TNAME}.testdata.temp
diff ${TNAME}.testdata ${TNAME}.testdata.temp >&2

#=============================================================================
# compare with baseline case
#=============================================================================

FILES="\
 ${TNAME}.out.0\
 ${TNAME}.out.1\
 ${TNAME}.out.2\
"

for i in $FILES
do
  echo "# Output file: $i"
  tail -21 $i | head -6
done > ${TNAME}.out

# Make sure that the output files are reasonable
CHECK_LINE="Complexity"
OUT_COUNT=`grep "$CHECK_LINE" ${TNAME}.out | wc -l`
SAVED_COUNT=`grep "$CHECK_LINE" ${TNAME}.saved | wc -l`
if [ "$OUT_COUNT" != "$SAVED_COUNT" ]; then
   echo "Incorrect number of \"$CHECK_LINE\" lines in ${TNAME}.out" >&2
fi

if [ -z $HYPRE_NO_SAVED ]; then
   #diff -U3 -bI"time" ${TNAME}.saved ${TNAME}.out >&2
   (../runcheck.sh ${TNAME}.out ${TNAME}.saved $RTOL $ATOL) >&2
fi

#=============================================================================
# remove temporary files
#=============================================================================

rm -f ${TNAME}.testdata*
