#!/bin/bash
source "./configuration.txt"

[ -d "${data_dir}" ] || mkdir "${data_dir}"

for seed in $(seq 1 ${seed_max}); do
    for mimo in ${mimo_values}; do
	    for ue in ${ue_values}; do
            while [ $(pgrep -f ${lte_sim} | wc -l) -ge ${max_concurrent_simulations} ]; do
                sleep 0.04
            done
            sleep 0.04
            echo "Launching simulation: MIMO configuration ${mimo}, ${ue} users, seed ${seed}"
			${5G_sim} test-calibration-scenarios 0.5 0.025 2000 ${ue} ${sched_type} ${speed} ${bandwidth} ${duration} ${mimo//-/\ } 46 9 5 12 15 17 25 20 2 0 ${seed} | gzip >"${data_dir}/sim_${ue}ue_${mimo}_UMa_seed${seed}.txt.gz" &
		done
	done
done

while pgrep -f ${5G_sim} >/dev/null; do
    sleep 2
done
