#!/usr/bin/gawk -f

{
	rate = $6;
	pos_x = $9;
	pos_y = $10;
	sum[pos_x][pos_y] += rate;
	count[pos_x][pos_y]++;
}

END {
	for (i in count) {
		for (j in count[i]){
			print sum[i][j]/count[i][j]
		}
	} 
}
