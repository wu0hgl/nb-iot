#!/bin/sh

SISO_FILE="simulation-siso.txt"
MIMO_FILE="simulation-mimo.txt"

# simulation command
../../5G-simulator f5g-demo1 suburban 0.25 400 50 30 17 1 1 1 1 >"${SISO_FILE}"&
../../5G-simulator f5g-demo1 suburban 0.25 400 50 30 17 8 8 9 1 >"${MIMO_FILE}"

# post-processing
#awk '/^PHY_RX/{x=$7} /^CALIBRATION_1/{print x,$5}' "${SISO_FILE}" >sinr_avg_siso.txt
awk '/^PHY_RX/{print $7,$11}' "${SISO_FILE}" >sinr_siso.txt
awk '/^PHY_RX/{print $7,$17*5/1000}' "${SISO_FILE}" >throughput_siso.txt
#awk '/^PHY_RX/{x=$7} /^CALIBRATION_1/{print x,$5}' "${MIMO_FILE}" >sinr_avg_mimo.txt
awk '/^PHY_RX/{print $7,$11}' "${MIMO_FILE}" >sinr_mimo.txt
awk '/^PHY_RX/{print $7,$17*5/1000}' "${MIMO_FILE}" >throughput_mimo.txt
awk '/^PHY_RX/{print $21,$7}' "${MIMO_FILE}" >position.txt

./user_position_postprocessing.awk "${SISO_FILE}"
./plot_positions.sh 0.25

#join sinr_avg_siso.txt sinr_avg_mimo.txt >sinr_avg.txt
#rm sinr_avg_siso.txt sinr_avg_mimo.txt
join sinr_siso.txt sinr_mimo.txt >sinr.txt
rm sinr_siso.txt sinr_mimo.txt
join throughput_siso.txt throughput_mimo.txt >throughput.txt
rm throughput_siso.txt throughput_mimo.txt

GNUPLOT_SCRIPT="plot.gplt"

echo "set terminal pngcairo font 'Helvetica,20' size 800,600" >"${GNUPLOT_SCRIPT}"

#echo "set output 'sinr_avg.png'" >>"${GNUPLOT_SCRIPT}"
#echo "set xlabel 'Distance (m)'" >>"${GNUPLOT_SCRIPT}"
#echo "set yrange [-10:25]" >>"${GNUPLOT_SCRIPT}"
#echo "set ylabel 'Wideband SINR (dB)'" >>"${GNUPLOT_SCRIPT}"
#echo "plot 'sinr_avg.txt' using 1:2 with lines title 'SISO', '' using 1:3 with lines title 'MIMO'" >>"${GNUPLOT_SCRIPT}"


echo "set output 'sinr.png'" >>"${GNUPLOT_SCRIPT}"
echo "set xlabel 'Distance (m)'" >>"${GNUPLOT_SCRIPT}"
echo "set yrange [-10:25]" >>"${GNUPLOT_SCRIPT}"
echo "set ylabel 'SINR (dB)'" >>"${GNUPLOT_SCRIPT}"
echo "plot 'sinr.txt' using 1:2 with lines title 'SISO', '' using 1:3 with lines title 'MIMO (8x8 TM9)'" >>"${GNUPLOT_SCRIPT}"


echo "set output 'Throughput.png'" >>"${GNUPLOT_SCRIPT}"
echo "set xlabel 'Distance (m)'" >>"${GNUPLOT_SCRIPT}"
echo "set yrange [0:1500]" >>"${GNUPLOT_SCRIPT}"
echo "set ylabel 'Throughput (Mbps)'" >>"${GNUPLOT_SCRIPT}"
echo "plot 'throughput.txt' using 1:2 with lines title 'SISO', '' using 1:3 with lines title 'MIMO (8x8 TM9)'" >>"${GNUPLOT_SCRIPT}"

echo "set output 'Position.png'" >>"${GNUPLOT_SCRIPT}"
echo "set xlabel 'Time (s)'" >>"${GNUPLOT_SCRIPT}"
echo "set yrange [0:200]" >>"${GNUPLOT_SCRIPT}"
echo "set ylabel 'Distance (m)'" >>"${GNUPLOT_SCRIPT}"
echo "plot 'position.txt' using 1:2 with lines notitle" >>"${GNUPLOT_SCRIPT}"

gnuplot "${GNUPLOT_SCRIPT}"

xdg-open 'userplot.png' &
xdg-open 'sinr.png' &
xdg-open 'Throughput.png' &
xdg-open 'Position.png' &
