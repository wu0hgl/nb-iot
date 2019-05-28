#!/bin/bash
source "./configuration.txt"

[ -d "${results_dir}" ] || mkdir "${results_dir}"

throughput_file="throughput.txt"
throughput_per_user_file="throughput_per_user.txt"
echo "nbUe ${mimo_descriptions}" >"${throughput_file}"
echo "nbUe ${mimo_descriptions}" >"${throughput_per_user_file}"
seed_max=3

for ue in ${ue_values}; do
	line_throughput="${ue}"
	line_throughput_per_user="${ue}"
	for mimo in ${mimo_values}; do
		data_files=""
		for seed in $(seq 1 ${seed_max}); do
			data_files="${data_files} ${data_dir}/sim_${ue}ue_${mimo}_UMa_seed${seed}.txt.gz"
		done
		line_throughput="${line_throughput} $(gunzip -c ${data_files} | awk -v seed=${seed_max} -f throughput.awk)"
		line_throughput_per_user="${line_throughput_per_user} $(gunzip -c ${data_files} | awk -v n=${ue} -v seed=${seed_max} -f throughput_per_user.awk)"
	done
	echo "${line_throughput}"          >>"${throughput_file}"
	echo "${line_throughput_per_user}" >>"${throughput_per_user_file}"
done

PLR_file="PLR.txt"
[ -f "${PLR_file}" ] && rm "${PLR_file}"
for ue in ${ue_values}; do
	printf "%d " $ue >> "${PLR_file}"
	for mimo in ${mimo_values}; do
		gunzip -c mimo-test_multiuser_data/sim_${ue}ue_${mimo}_UMa_seed*.txt.gz | awk '/^TX/{tx++} /^RX/{rx++} END{printf("%f ",(tx-rx)/tx*100)}' >> "${PLR_file}"
	done
	printf "\n" >> "${PLR_file}"
done
