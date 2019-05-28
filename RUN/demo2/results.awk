#!/usr/bin/awk -f

BEGIN {
	wrong_blocks = 0
	total_blocks = 0	
}

/Created Cell/{
	radius = $9
	area = radius*radius*sqrt(3)*1.5*7
}

/^PHY_RX/{
	user = $5
	bits = $17
	error = $19
	
	if(error==1){
		wrong_blocks++
	} else {
		tot_phy_bits += bits
		tot_phy_count++
		user_phy_bits[user] += bits
		user_phy_count[user]++
	}
	total_blocks++
}

END{
	print "Aggregate goodput:", tot_phy_bits/duration/1000/1000,"Mbps"
	
	for(i in user_phy_count){
		user_phy_goodput[i]=user_phy_bits[i]/duration/1000/1000
		avg_user_goodput_sum += user_phy_goodput[i]
		avg_user_goodput_count++
	}
	print "Average user goodput:", avg_user_goodput_sum/avg_user_goodput_count,"Mbps"
	
	print "Packet Loss Ratio:", wrong_blocks/total_blocks*100,"%"
	
	print "Traffic density @ 20 MHz:", tot_phy_bits/duration/area/1000/1000,"Mbps/km2"
	
	print "Traffic density @ 100 MHz:", tot_phy_bits*5/duration/area/1000/1000,"Mbps/km2"
} 
