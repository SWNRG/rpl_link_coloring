#!/usr/bin/env gnuplot

set xrange  [0:60]
set yrange [0:30000] 
set grid ytics
set grid xtics


# get the name of the experiment folder. E.g. 1s-10m-25f 
# new var, only the name of the containing folder
titolo1 = system ("pwd | rev | cut -d '/' -f 2 | rev")
titolo2 = system ("pwd | rev | cut -d '/' -f 1 | rev")

titlo = titolo1.'-RTT-'.titolo2[4:]
set title "controlExperiment-FourNodesSend-IN-Row"

set term png
set output "controlExperiment-AllNodesInRow".'.png'

set xlabel 'Time (min)'
set ylabel 'RTT (ms)'


#FILES = system("ls RTT*.csv")
#LABEL = system("ls *.log | sed -e 's/-RX100%-2hr.csc.log//'"
#N=`awk 'NR==1 {print NF}' Data.txt`
#plot for [i=1:words(FILES)] word(FILES,i) u 2 t word(LABEL,i) with linespoints\
plot for [i=1:words(FILES)] word(FILES,i) u 2 t word(LABEL,i) with linespoints\

N=`awk 'NR==1 {print NF}' experiment2-FourNodesSend.log`
unset key
set key title "Nodes: " autotitle columnheader bottom horizontal center
plot for [i=2:N] "controlExperiment-FourNodesSend-IN-Row.log" u 1:i with lines

