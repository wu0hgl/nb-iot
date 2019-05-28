#!/usr/bin/awk -f
BEGIN {
	# these values should be provided from the command line, e.g. -v num_points=400
    if(num_points=="") {num_points=300;}
    if(value_min=="") {value_min=0;}
    if(value_max=="") {value_max=3000;}
    
    samples=0;
}

{
	current_value=$1;
    value_index=int((current_value-value_min)*num_points/(value_max-value_min));
    value_index=(value_index>num_points ? num_points : value_index);
    value_index=(value_index<0 ? 0 : value_index);
    value[value_index]++;
    samples++;
}

END {
    for (i=0; i<=num_points; i++) {
	value[i]=value[i]/samples + value[i-1];
	
	print i*(value_max-value_min)/num_points+value_min, value[i];
    }
}
