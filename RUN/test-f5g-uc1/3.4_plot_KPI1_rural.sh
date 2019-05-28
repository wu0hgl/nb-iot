#!/bin/bash
. "./configuration.txt"
. "./configuration_rural.txt"

# set file names and parameters
GNUPLOT_FILE="rural.gplt"
PLOT_SIZE=1600,1200
KPI1_graph="${results_dir}/KPI1_rural.png"
col_count=$(echo $speed_values | awk '{print NF+1}')
xrange="[40:110]"
yrange="[0:7]"
target_y=5

echo "Plotting traffic density (rural scenario)"

# set general graph properties
echo "set terminal pngcairo size ${PLOT_SIZE} enhanced dashed font \"Helvetica,26\"" > "${GNUPLOT_FILE}"
echo "set grid" >> "${GNUPLOT_FILE}"
echo "set key top left" >> "${GNUPLOT_FILE}"
echo "set style data linespoints" >> "${GNUPLOT_FILE}"

# describe graph
echo "set output \"${KPI1_graph}\"" >> "${GNUPLOT_FILE}"
echo "set title 'Traffic density (rural scenario)'" >> "${GNUPLOT_FILE}"
echo "set xlabel 'User density (ue/km^2)'" >> "${GNUPLOT_FILE}"
echo "set ylabel 'Traffic density (Gbit/s/km^2)'" >> "${GNUPLOT_FILE}"
echo "set xrange ${xrange}" >> "${GNUPLOT_FILE}"
echo "set yrange ${yrange}" >> "${GNUPLOT_FILE}"
echo "plot \\"  >> "${GNUPLOT_FILE}"
echo "for [COL=2:$col_count] \"${KPI1_data}\" using 1:COL pointsize 3 dashtype COL linewidth 3 title columnheader,\\" >> "${GNUPLOT_FILE}"
echo "" >> "${GNUPLOT_FILE}"
#echo "${target_y} title \"FANTASTIC-5G target\" linecolor black" >> "${GNUPLOT_FILE}"

# plot graph
gnuplot "${GNUPLOT_FILE}" && rm "${GNUPLOT_FILE}"
