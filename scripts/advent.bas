type room_t
	dim string name
	dim string description
	dim string exit_n
	dim string exit_s
	dim string exit_e
	dim string exit_w
	dim string action1
	dim string takeaction1
end type

dim room_t[] rooms(7)
dim croom

function buildrooms()
	redim rooms, 7
	# -----
	let rooms[0].name = "entrance"
	let rooms[0].description = "A dark cave mouth yawns forbodingly here."
	let rooms[0].exit_n = "cave1"
	let rooms[0].exit_w = "forest1"
	# -----
	# let rooms[1].name = "forest1"
	# let rooms[1].description = "Dusk bathes this forest clearing in warm, mottled sunlight."
	# let rooms[1].exit_e = "entrance"
	# # -----
	# let rooms[2].name = "cave1"
	# let rooms[2].description = "It's pitch black. If only I had a torch..."
	# let rooms[2].action1 = "torch"
	# let rooms[2].takeaction1 = "You light the torch. The room brightens up."
	# # -----
	# let rooms[3].name = "cave2"
	# let rooms[3].description = "A damp and mysterious cave. A torch flickers here."
	# let rooms[3].exit_e = "goblin1"
	# let rooms[3].exit_w = "dragon1"
	# # -----
	# let rooms[4].name = "dragon1"
	# let rooms[4].description = "Oops! A horrible dragon is here! A half-eaten knight lays near you, holding a sword."
	# let rooms[4].action1 = "sword"
	# let rooms[4].exit_e = "cave2"
	# let rooms[4].takeaction1 = "You thought you were tougher than a knight? No, you were weak an tender, and rather succulent when roasted, according the dragon."
	# # -----
	# let rooms[5].name = "goblin1"
	# let rooms[5].description = "A snarling hobgobling chews some filthy meat here. He sits with his back to you, next to a rather large rock."
	# let rooms[5].exit_w = "cave2"
	# let rooms[5].action1 = "rock"
	# let rooms[5].takeaction1 = "With a hefty crack you bring the rock down, splitting his skull. A fitting end. There was something behind him... a crack in the wall."
	# # -----
	# let rooms[6].name = "exit"
	# let rooms[6].description = "You escape from the cave into the medow beyond. Your nightmare adventure is finally at an end! You roll around jubilantly in the grass, disturbing the meadow badgers, which eat you."
end function

function mainloop()
	dim string[] ass
	# dim string inp, dirstr
	# dim string[] cmd, exits
	# dim do_look = 1
end function

function main()
	dim iswin
	call buildrooms()
	print "Welcome to the depths."
	let iswin = mainloop()
	print ""
	if iswin
		print "You win!"
	else
		print "You are dead."
	end if
	print "Thanks for playing."
end function