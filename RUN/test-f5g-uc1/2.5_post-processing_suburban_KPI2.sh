#!/bin/bash
. "./configuration.txt"
. "./configuration_suburban.txt"

[ -d "${results_dir}" ] || mkdir "${results_dir}"
[ -f "${KPI2_data}"   ] && rm    "${KPI2_data}"

bitrate_max="$(echo $bitrate_values | awk '{print $NF}')"
temp_file="${results_dir}/temp.txt"

for speed in ${speed_values}; do
	
	echo "Calculating delay with speed = ${speed} km/h (suburban scenario)"
	echo "density \"speed ${speed} km/h\"" > "${temp_file}"

	for density in ${density_values}; do
		for file in "${data_dir}/sim_uc1_suburban_density_${density}ue_km2_${speed}kmh_seed"* ; do
			gunzip -c  "${file}" \
				| awk "
					/^RX/{
						delay[\$12] += \$14;
						samples[\$12]++;
					}
					END{
						for (i in delay){
							print delay[i]/samples[i];
						}
					}
					"
		done \
			| awk "
				{
					sum+=\$1;
					count++;
				}
				END{
					print $density, sum/count;
				}
				" \
			>> "${temp_file}"
	done
	if [ -f "${KPI2_data}" ]; then
		mv "${KPI2_data}" "${KPI2_data}.old"
		join "${KPI2_data}.old" "${temp_file}" > "${KPI2_data}"
		rm "${KPI2_data}.old" "${temp_file}"
	else
		mv "${temp_file}" "${KPI2_data}"
	fi
done
