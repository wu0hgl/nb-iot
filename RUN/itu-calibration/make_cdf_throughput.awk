#!/usr/bin/awk -f
BEGIN {
    num_points=300;
    throughput_min=0;
    throughput_max=2;
    samples=0;
}

/^Created UE/ {
	last_transmission_time[$5]=0;
}

/^CALIBRATION_1/ {
    throughput_index=int(($9/($11-last_transmission_time[$13])-throughput_min)*num_points/(throughput_max-throughput_min));
    throughput_index=(throughput_index>num_points ? num_points : throughput_index);
    throughput_index=(throughput_index<0 ? 0 : throughput_index);
    throughput[throughput_index]++;
    samples++;
    last_transmission_time[$13]=$11;
}

END {
    for (i=0; i<=num_points; i++) {
	throughput[i]=throughput[i]/samples + throughput[i-1];
	
	print i*(throughput_max-throughput_min)/num_points+throughput_min, throughput[i];
    }
}
