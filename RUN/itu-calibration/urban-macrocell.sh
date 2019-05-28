#!/bin/sh

SCEN="Urban_Macro_cell_1a_rand"
 
OUT_DIR="./output/$SCEN"
rm -r $OUT_DIR
mkdir $OUT_DIR


# Urban Macro-cell parameters

ISD=0.5 # Inter Site Distance in km
MINDIST=0.025 # Minimum Distance between UT and Serving Cell in km
CF=2000 # Carrier Frequency in MHz
NBUE=210 # Number of User Equipment in main cell
SCHED_TYPE=8 # Scheduler Type - Round Robin
SPEED=30 # Speed in km/h
BANDWIDTH=10 # Bandwidth FDD in MHz
DURATION=1 #60
ENBTX=1 # Number of ENodeB antenna elements (Tx)
UERX=2 # Number of User Equipment antenna elements (Rx)
TXMODE=1
TXPOWER=46 # ENodeB transmit power for 10 MHz
PLOSS_M=9 # Penetration Loss in dB
PLOSS_SD=5
ETILT=12 # ENodeB Elettrical Antenna Downtilt
VBEAMWIDTH=15 # Antenna Vertical Beam Width
A_GAIN=17 # Antenna Gain
A_HEIGHT=25 #Antenna Height
A_ATTENUATION=20 # Antenna Horizontal Attenuation
BS_FEEDER_LOSS=2 # Base Station feeder loss
C_MODEL=0 # Channel Model (0 - URBAN_MACROCELL_IMT; 1 - RURAL_MACROCELL_IMT; 2 - SUB_URBAN_MACROCELL)

# Run simulation

echo SIMULATING $SCEN SCENARIO

COUNT=1
N_SEED=16
RUN="../../5G-simulator"
max_concurrent_simulations=4

until [ $COUNT -gt $N_SEED ];	do

	while [ $(pgrep -f ${RUN} | wc -l) -ge ${max_concurrent_simulations} ]; do
	    sleep 0.2
    done
    sleep 0.2
	
	file=${OUT_DIR}/test_${SCEN}_Seed_${COUNT}
	echo Simulating Seed ${COUNT}
#	$RUN itu-calibration $ISD $MINDIST $CF $NBUE $SCHED_TYPE $SPEED $BANDWIDTH $DURATION $ENBTX $UERX $TXMODE $TXPOWER $PLOSS_M $PLOSS_SD $ETILT $VBEAMWIDTH $A_GAIN $A_HEIGHT $A_ATTENUATION $BS_FEEDER_LOSS $C_MODEL $COUNT > ${file} &
	$RUN urban-macrocell-itu $NBUE $DURATION $COUNT > ${file} &
	
	COUNT=$(($COUNT+1))
done

while pgrep -f ${RUN} >/dev/null; do
    sleep 0.2
done


echo Making CDF
./make_cdf_pathgain.awk "${OUT_DIR}/test_${SCEN}_Seed_"* > "${OUT_DIR}/cdf_pathgain_step1_${SCEN}"
./make_cdf_sinr.awk "${OUT_DIR}/test_${SCEN}_Seed_"* > "${OUT_DIR}/cdf_sinr_step1_${SCEN}"
./make_cdf_sinr_ff.awk "${OUT_DIR}/test_${SCEN}_Seed_"* > "${OUT_DIR}/cdf_sinr_${SCEN}"
./make_cdf_throughput.awk "${OUT_DIR}/test_${SCEN}_Seed_"* > "${OUT_DIR}/cdf_throughput_media_${SCEN}"

echo Making graphs
gnuplot plot_urban.gplt

echo END SIMULATION
