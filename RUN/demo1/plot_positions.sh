#!/bin/bash

isd="$1"

. "configuration.txt"

GNUPLOT_SCRIPT="plot.gplt"

CELL_POSITIONS_FILE="userposition/cell_positions.txt"
USER_POSITIONS_FILE="userposition/final_user_positions.txt"

echo "set terminal pngcairo size 1200,1200 font 'Helvetica,24'" > "${GNUPLOT_SCRIPT}"
echo "set output '${user_plot_file}'" >> "${GNUPLOT_SCRIPT}"
echo "set xlabel 'Position on X axis (m)'" >> "${GNUPLOT_SCRIPT}"
echo "set ylabel 'Position on Y axis (m)'" >> "${GNUPLOT_SCRIPT}"
echo "set xrange [-${isd}*2.75*1000:${isd}*2.75*1000]" >> "${GNUPLOT_SCRIPT}"
echo "set yrange [-${isd}*2.75*1000:${isd}*2.75*1000]" >> "${GNUPLOT_SCRIPT}"
echo "set key off" >> "${GNUPLOT_SCRIPT}"
echo "unset colorbox" >> "${GNUPLOT_SCRIPT}"
echo "set palette defined ( 0 'white', 1 'black' ) " >> "${GNUPLOT_SCRIPT}"

echo "plot\\" >> "${GNUPLOT_SCRIPT}"
echo "  '${CELL_POSITIONS_FILE}' using 1:2:3 with labels point pt 8 ps 8 offset char -0.4,-0.3 title columnheader, \\" >> "${GNUPLOT_SCRIPT}"
for i in "userposition/position_cell"*.txt; do
    echo "  '$i' with lines linecolor rgb '#8888ff' notitle,\\" >> "${GNUPLOT_SCRIPT}"
done
for i in "userposition/position_user"*.txt; do
	echo "  '$i' with lines linecolor palette title columnheader,\\" >> "${GNUPLOT_SCRIPT}"
done
echo "  '${USER_POSITIONS_FILE}' using 1:2:3 with labels font 'Helvetica,18' point ps 2 pt 7 offset char 0.3,0.3 title columnheader,\\" >> "${GNUPLOT_SCRIPT}"
echo "    \"<echo $(awk "BEGIN{print ${isd}*1.5*1000, ${isd}*2.5*1000}"  </dev/null ) \'BaseStation\'\"  using 1:2:3 with labels point pt 8 ps 4 offset char 6,-0.25,\\" >> "${GNUPLOT_SCRIPT}"
echo "    \"<echo $(awk "BEGIN{print ${isd}*1.5*1000, ${isd}*2.25*1000}" </dev/null ) \'UserTerminal\'\"  using 1:2:3 with labels point pt 7 ps 2 offset char 6,-0.25" >> "${GNUPLOT_SCRIPT}"

gnuplot "${GNUPLOT_SCRIPT}" && rm "${GNUPLOT_SCRIPT}"
