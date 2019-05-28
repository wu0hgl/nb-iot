#!/bin/bash
. "./configuration.txt"
. "./configuration_rural.txt"

time_resolution=0.1
space_resolution=5
bitrate_max=50
KPI0_data="${results_dir}/KPI0_data_rural.txt"

[ -d "${results_dir}" ] || mkdir "${results_dir}"

isd_max="$(echo $isd_values | awk '{print $NF}')"
density_max="$(echo $density_values | awk '{print $NF}')"

[ -f "${KPI0_cdf_data_time}" ] || rm "${KPI0_cdf_data_time}"
[ -f "${KPI0_cdf_data_time}" ] || rm "${KPI0_cdf_data_space}"

for density in $density_values; do
	gunzip -c "${data_dir}/sim_uc1_rural_isd_${isd_max}km_density_${density}ue_km2_${bitrate_max}Mbps_seed"* \
		| ./KPI0.awk -v time_resolution=${time_resolution} -v space_resolution=${space_resolution}\
		> "${KPI0_data}"
		
	echo "cdf \"$density ue/km2\"" > "${KPI0_cdf_data_time}.tmp"
	./KPI0_time.awk "${KPI0_data}" \
		| awk '{print $1/1000}' \
		| ./make_cdf.awk -v value_max=${bitrate_max} \
		>> "${KPI0_cdf_data_time}.tmp"
	if [ -f "${KPI0_cdf_data_time}" ]; then
		mv "${KPI0_cdf_data_time}" "${KPI0_cdf_data_time}.old"
		join "${KPI0_cdf_data_time}.old" "${KPI0_cdf_data_time}.tmp" > "${KPI0_cdf_data_time}"
		rm "${KPI0_cdf_data_time}.old" "${KPI0_cdf_data_time}.tmp"
	else
		mv "${KPI0_cdf_data_time}.tmp" "${KPI0_cdf_data_time}"
	fi
	
	echo "cdf \"$density ue/km2\"" > "${KPI0_cdf_data_space}.tmp"
	./KPI0_space.awk "${KPI0_data}" \
		| awk '{print $1/1000}' \
		| ./make_cdf.awk -v value_max=${bitrate_max} \
		>> "${KPI0_cdf_data_space}.tmp"
	if [ -f "${KPI0_cdf_data_space}" ]; then
		mv "${KPI0_cdf_data_space}" "${KPI0_cdf_data_space}.old"
		join "${KPI0_cdf_data_space}.old" "${KPI0_cdf_data_space}.tmp" > "${KPI0_cdf_data_space}"
		rm "${KPI0_cdf_data_space}.old" "${KPI0_cdf_data_space}.tmp"
	else
		mv "${KPI0_cdf_data_space}.tmp" "${KPI0_cdf_data_space}"
	fi
done
