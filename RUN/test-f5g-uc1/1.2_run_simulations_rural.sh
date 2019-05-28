#!/bin/bash
source "./configuration.txt"
source "./configuration_rural.txt"

[ -d "${data_dir}" ] || mkdir "${data_dir}"

for seed in $(seq 1 ${seed_max}); do
    for speed in ${speed_values}; do
        for density in ${density_values}; do
            while [ $(pgrep -f ${lte_sim} | wc -l) -ge ${max_concurrent_simulations} ]; do
                sleep 0.2
            done
            sleep 0.2
            echo "Launching simulation: isd ${isd} km, density ${density} ue/km2, bitrate ${bitrate} Mbps, speed ${speed} km/h, seed ${seed}"
	        ${lte_sim} f5g-uc1 rural ${isd} $((density/5)) ${bitrate} ${speed} ${duration} ${seed} | gzip >"${data_dir}/sim_uc1_rural_density_${density}ue_km2_${speed}kmh_seed${seed}.txt.gz" &
	    done
    done
done

while pgrep -f ${lte_sim} >/dev/null; do
    sleep 0.2
done

