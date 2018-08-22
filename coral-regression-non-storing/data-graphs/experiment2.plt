#!/usr/bin/env gnuplot

set xrange  [0:60]
set yrange [0:20000] 
set grid ytics
set grid xtics


# get the name of the experiment folder. E.g. 1s-10m-25f 
# new var, only the name of the containing folder
titolo1 = system ("pwd | rev | cut -d '/' -f 2 | rev")
titolo2 = system ("pwd | rev | cut -d '/' -f 1 | rev")

titlo = titolo1.'-RTT-'.titolo2[4:]
set title "experiment2-FourNodesSend"

set term png
set output "experiment2-FourNodesSend".'.png'

set xlabel 'Time (min)'
set ylabel 'RTT (ms)'

set arrow from 20, graph 0 to 20, graph 1 nohead lc rgb "red" 
set label "          Change\n  Objective Function"  at  21,19000 textcol rgb "red"
set arrow from 40, graph 0 to 40, graph 1 nohead lc rgb "red" 

#FILES = system("ls RTT*.csv")
#LABEL = system("ls *.log | sed -e 's/-RX100%-2hr.csc.log//'"
#N=`awk 'NR==1 {print NF}' Data.txt`
#plot for [i=1:words(FILES)] word(FILES,i) u 2 t word(LABEL,i) with linespoints\
plot for [i=1:words(FILES)] word(FILES,i) u 2 t word(LABEL,i) with linespoints\

N=`awk 'NR==1 {print NF}' experiment2-FourNodesSend.log`
unset key
set key title "Nodes: " autotitle columnheader bottom horizontal center
plot for [i=2:N] "experiment2-FourNodesSend.log" u 1:i with lines

