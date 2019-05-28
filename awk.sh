#!/bin/bash

# schedType = atoi(argv[2]);			1/2
# double dur = atoi(argv[3]);  			150
# double radius = atof(argv[4]);		1
# int nbUE = atoi(argv[5]);				20/40/60
# double bandwidth = atof(argv[6]);		
# int carriers = atoi(argv[7]);
# double spacing = atof(argv[8]);		3.75/15
# int tones = atoi(argv[9]);			3/6/12
# int CBR_interval = atoi(argv[10]);	60
# int CBR_size = atoi(argv[11]);		128/256
# int seed;

export LTE_SIM_SCHEDULER_DEBUG_LOG=1

#					  schedType  dur  radius  nbUE  bandwidth  carriers  spacing  tones  CBR_interval  CBR_size  seed
#echo "1200 UEs"
#./5G-simulator 	nbCell 	  1      150     1    1200      5         1        3.75      1        60          128      1 > 1200_UEs.out
#echo "2400 UEs"
#./5G-simulator 	nbCell 	  1      150     1    2400      5         1        3.75      1        60          128      1 > 2400_UEs.out
#echo "3600 UEs"
#./5G-simulator 	nbCell 	  1      150     1    3600      5         1        3.75      1        60          128      1 > 3600_UEs.out

# post-processing
printf "processing\n"
awk '/^RX/{print $0}' "sim.out" > sim.txt
#awk '/^RACH/{print $12,$6,$8,$10}' "2400_UEs.out" > RACH_2400_UEs.txt
#awk '/^RACH/{print $12,$6,$8,$10}' "3600_UEs.out" > RACH_3600_UEs.txt
