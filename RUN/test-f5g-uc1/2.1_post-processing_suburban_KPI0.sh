#!/bin/bash
. "./configuration.txt"
. "./configuration_suburban.txt"

[ -d "${results_dir}" ] || mkdir "${results_dir}"
[ -f "${KPI0_data}"   ] && rm    "${KPI0_data}"

bitrate_max="$(echo $bitrate_values | awk '{print $NF}')"
temp_file="${results_dir}/temp.txt"

for speed in ${speed_values}; do
	
	echo "Calculating user experienced data rate with speed = ${speed} km/h (suburban scenario)"
	echo "density \"speed ${speed} km/h\"" > "${temp_file}"
	
	for density in ${density_values}; do
		for file in "${data_dir}/sim_uc1_suburban_density_${density}ue_km2_${speed}kmh_seed"* ; do
			gunzip -c  "${file}" \
				| awk "
					/^PHY_RX/{
						bits[\$5] += \$17;
					}
					END{
						for (i in bits){
							print bits[i]/${duration}/1000/1000;
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
	if [ -f "${KPI0_data}" ]; then
		mv "${KPI0_data}" "${KPI0_data}.old"
		join "${KPI0_data}.old" "${temp_file}" > "${KPI0_data}"
		rm "${KPI0_data}.old" "${temp_file}"
	else
		mv "${temp_file}" "${KPI0_data}"
	fi
done
