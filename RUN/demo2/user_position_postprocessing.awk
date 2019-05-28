#!/usr/bin/awk -f

BEGIN{
	ue_counter = 0
	cell_counter = 0
	data_dir="userposition"
	make_data_dir="mkdir -p " data_dir " 2>/dev/null"
	system(make_data_dir)
	close(make_data_dir)
	clean_data_dir="rm " data_dir "/* 2>/dev/null"
	system(clean_data_dir)
	close(clean_data_dir)
	print "x","y" >"userposition/cell_positions.txt"
}

/^Created Cell/{
	cell_id = substr($4, 0, length($4) - 1)
	cell_x[cell_id] = $6
	cell_y[cell_id] = $7
	radius = $9*1000;
	print cell_x[cell_id], cell_y[cell_id], cell_id >>"userposition/cell_positions.txt"
	print cell_x[cell_id],                    cell_y[cell_id]           >"userposition/position_cell"cell_id".txt"
	print cell_x[cell_id]+radius*sqrt(3)/6,   cell_y[cell_id]+radius/2 >>"userposition/position_cell"cell_id".txt"
	print cell_x[cell_id]+radius*sqrt(3)/2,   cell_y[cell_id]+radius/2 >>"userposition/position_cell"cell_id".txt"
	print cell_x[cell_id]+radius*sqrt(3)*2/3, cell_y[cell_id]          >>"userposition/position_cell"cell_id".txt"
	print cell_x[cell_id]+radius*sqrt(3)/2,   cell_y[cell_id]-radius/2 >>"userposition/position_cell"cell_id".txt"
	print cell_x[cell_id]+radius*sqrt(3)/6,   cell_y[cell_id]-radius/2 >>"userposition/position_cell"cell_id".txt"
	print cell_x[cell_id],                    cell_y[cell_id]          >>"userposition/position_cell"cell_id".txt"
	print cell_x[cell_id]+radius*sqrt(3)/6,   cell_y[cell_id]-radius/2 >>"userposition/position_cell"cell_id".txt"
	print cell_x[cell_id],                    cell_y[cell_id]-radius   >>"userposition/position_cell"cell_id".txt"
	print cell_x[cell_id]-radius*sqrt(3)/3,   cell_y[cell_id]-radius   >>"userposition/position_cell"cell_id".txt"
	print cell_x[cell_id]-radius*sqrt(3)/2,   cell_y[cell_id]-radius/2 >>"userposition/position_cell"cell_id".txt"
	print cell_x[cell_id]-radius*sqrt(3)/3,   cell_y[cell_id]          >>"userposition/position_cell"cell_id".txt"
	print cell_x[cell_id],                    cell_y[cell_id]          >>"userposition/position_cell"cell_id".txt"
	print cell_x[cell_id]-radius*sqrt(3)/3,   cell_y[cell_id]          >>"userposition/position_cell"cell_id".txt"
	print cell_x[cell_id]-radius*sqrt(3)/2,   cell_y[cell_id]+radius/2 >>"userposition/position_cell"cell_id".txt"
	print cell_x[cell_id]-radius*sqrt(3)/3,   cell_y[cell_id]+radius   >>"userposition/position_cell"cell_id".txt"
	print cell_x[cell_id],                    cell_y[cell_id]+radius   >>"userposition/position_cell"cell_id".txt"
	print cell_x[cell_id]+radius*sqrt(3)/6,   cell_y[cell_id]+radius/2 >>"userposition/position_cell"cell_id".txt"
	print cell_x[cell_id],                    cell_y[cell_id]          >>"userposition/position_cell"cell_id".txt"
	cell_counter++
}

/^Created UE/{
	ue_id[ue_counter] = $5
	print 0, ue_id[ue_counter], 0 > "userposition/position_user"ue_id[ue_counter]".txt"
	ue_counter++
}

/^ UserPosition X/ {
	for(i=0;i<ue_counter;i++){
		ue_x[i] = $(i+5)
	}
}

/^ UserPosition Y/ {
	time=substr($4, 0, length($4) - 1)
	for(i=0;i<ue_counter;i++){
		ue_y[i] = $(i+5)
		print ue_x[i], ue_y[i], time >>"userposition/position_user"ue_id[i]".txt"
	}
}

END {
	print "x", "y", "id" >"userposition/final_user_positions.txt"
	for(i=0;i<ue_counter;i++){
		print ue_x[i], ue_y[i], ue_id[i] >>"userposition/final_user_positions.txt"
	}
}
