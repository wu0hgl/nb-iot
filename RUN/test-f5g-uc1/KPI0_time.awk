#!/usr/bin/gawk -f

{
	rate = $6;
	time = $4;
	sum[time] += rate;
	count[time]++;
}

END {
	for (i in count) {
		print sum[i]/count[i]
	} 
}
