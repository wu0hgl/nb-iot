/^TX/ {
    duration=$14;
    src[$4]=$10+1;
}

/^RX/ {
	if(src[$4]>0 && src[$4]<4){
		total+=$8;
	}
}

END {
	print total*8/duration/seed/1000/1000/n;
}
