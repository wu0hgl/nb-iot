#!/usr/bin/awk -f

function round(value,precision){
	value=value/precision;
    if(value>=0){
		return (value<=int(value)+0.5 ? int(value) : int(value)+1)*precision;
    } else {
		return (value<int(value)-0.5 ? int(value)-1 : int(value))*precision;
    }
}

BEGIN {
	if(time_resolution=="") time_resolution = 0.1; # seconds
	if(space_resolution=="") space_resolution = 10; # meters
	
	new_time_interval = 1;
	#time_interval_bits = 0;
}

/^PHY_RX/ {
		if(new_time_interval==1){
			time_interval_start = $21;
			time_interval_bits[$5] = $17;
			new_time_interval = 0;
#print "starting new interval"
		} else {
			time_interval_duration = $21 - time_interval_start;
			if(time_interval_duration >= time_resolution || time_interval_duration < 0){
				for (i in time_interval_bits) {
					print "ue", i, "time", time_interval_start,
						"rate", time_interval_bits[i]/time_interval_duration/1000, "kbps",
						"position", position_x[i], position_y[i];
					delete time_interval_bits[i];
				}
				new_time_interval = 1;
			}
			time_interval_bits[$5] += $17;
#print "ue",$5,"adding", $17, "bits, total",  time_interval_bits[$5]
			position_x[$5] = round($7,10);
			position_y[$5] = round($9,10);
		}
}
