#!/usr/bin/awk -f
BEGIN {
    num_points=300;
    sinr_min=-140;
    sinr_max=-40;
    samples=0;
}

/^CALIBRATION_1/ {
    sinr_index=int(($3-sinr_min)*num_points/(sinr_max-sinr_min));
    sinr_index=(sinr_index>num_points ? num_points : sinr_index);
    sinr_index=(sinr_index<0 ? 0 : sinr_index);
    sinr[sinr_index]++;
    samples++;
}

END {
    for (i=0; i<=num_points; i++) {
	sinr[i]=sinr[i]/samples + sinr[i-1];
	
	print i*(sinr_max-sinr_min)/num_points+sinr_min, sinr[i];
    }
}
