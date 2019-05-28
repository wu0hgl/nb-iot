#!/bin/bash

. configuration.txt

printf "Type the inter-site distance, in meters (e.g. 250): "
read isd
# convert to km
isd="$(awk "BEGIN{print ${isd}/1000}" </dev/null)"

printf "Type the user density, in ue/km2 (e.g 400): "
read density

printf "Type the user speed, in km/h (e.g. 30): "
read speed

printf "Launching simulation..."
${lte_sim} f5g-uc1 suburban ${isd} $((density/5)) 50 ${speed} ${duration} ${seed} >"${simulation_file}" &
x-terminal-emulator -e ./follow.sh >/dev/null &
while pgrep -f ${lte_sim} 1>/dev/null 2>/dev/null; do
	sleep 1
done
printf "done\n"

printf "Plotting user position..."
./user_position_postprocessing.awk "${simulation_file}"
./plot_positions.sh "${isd}"
printf "done\n"
xdg-open "${user_plot_file}" &

echo "Computing metrics..."
echo ""
./results.awk -v duration=${duration} "${simulation_file}"
