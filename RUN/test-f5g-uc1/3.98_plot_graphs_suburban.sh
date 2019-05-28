#!/bin/bash
. "./configuration.txt"
. "./configuration_suburban.txt"

# set file names and parameters
GNUPLOT_FILE="suburban.gplt"
PLOT_SIZE=1600,1200
KPI0_cdf_time_graph="${results_dir}/KPI0_cdf_time_suburban.png"
KPI0_cdf_space_graph="${results_dir}/KPI0_cdf_space_suburban.png"

# set general graph properties
echo "set terminal pngcairo size ${PLOT_SIZE} enhanced font \"Helvetica,20\"" > "${GNUPLOT_FILE}"
echo "set grid" >> "${GNUPLOT_FILE}"
echo "set key bottom right" >> "${GNUPLOT_FILE}"

# describe graph of CDF over time
echo "set output \"${KPI0_cdf_time_graph}\"" >> "${GNUPLOT_FILE}"
echo "set title 'CDF of user experienced data rate over time (suburban scenario)'" >> "${GNUPLOT_FILE}"
echo "set xlabel 'User experienced data rate (Mbps)'" >> "${GNUPLOT_FILE}"
echo "plot \\"  >> "${GNUPLOT_FILE}"
col_count=$(echo $density_values | awk '{print NF+1}')
echo "for [COL=2:$col_count] \"${KPI0_cdf_data_time}\" using 1:COL with lines title columnheader" >> "${GNUPLOT_FILE}"

# describe graph of CDF over space
echo "set output \"${KPI0_cdf_space_graph}\"" >> "${GNUPLOT_FILE}"
echo "set title 'CDF of user experienced data rate over space (suburban scenario)'" >> "${GNUPLOT_FILE}"
echo "set xlabel 'User experienced data rate (Mbps)'" >> "${GNUPLOT_FILE}"
echo "plot \\"  >> "${GNUPLOT_FILE}"
col_count=$(echo $density_values | awk '{print NF+1}')
echo "for [COL=2:$col_count] \"${KPI0_cdf_data_space}\" using 1:COL with lines title columnheader" >> "${GNUPLOT_FILE}"

# plot graphs
gnuplot "${GNUPLOT_FILE}" && rm "${GNUPLOT_FILE}"
