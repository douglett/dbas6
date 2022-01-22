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

dim room_t[] rooms
dim croom = 0


function buildrooms()
	dim room_t r
	# -----
	let r.name = "entrance"
	let r.description = "A dark cave mouth yawns forbodingly here."
	let r.exit_n = "cave1"
	let r.exit_w = "forest1"
	push rooms, r
	# -----
	let r.name = "forest1"
	let r.description = "Dusk bathes this forest clearing in warm, mottled sunlight."
	let r.exit_e = "entrance"
	push rooms, r
	# -----
	let r.name = "cave1"
	let r.description = "It's pitch black. If only I had a torch..."
	let r.action1 = "torch"
	let r.takeaction1 = "You light the torch. The room brightens up."
	push rooms, r
	# -----
	let r.name = "cave2"
	let r.description = "A damp and mysterious cave. A torch flickers here."
	let r.exit_e = "goblin1"
	let r.exit_w = "dragon1"
	push rooms, r
	# -----
	let r.name = "dragon1"
	let r.description = "Oops! A horrible dragon is here! A half-eaten knight lays near you, holding a sword."
	let r.action1 = "sword"
	let r.exit_e = "cave2"
	let r.takeaction1 = "You thought you were tougher than a knight? No, you were weak an tender, and rather succulent when roasted, according the dragon."
	push rooms, r
	# -----
	let r.name = "goblin1"
	let r.description = "A snarling hobgobling chews some filthy meat here. He sits with his back to you, next to a rather large rock."
	let r.exit_w = "cave2"
	let r.action1 = "rock"
	let r.takeaction1 = "With a hefty crack you bring the rock down, splitting his skull. A fitting end. There was something behind him... a crack in the wall."
	push rooms, r
	# -----
	let r.name = "exit"
	let r.description = "You escape from the cave into the medow beyond. Your nightmare adventure is finally at an end! You roll around jubilantly in the grass, disturbing the meadow badgers, which eat you."
	push rooms, r
end function


function split(string str, string[] vs)
	dim i
	# for i = 0 to 
end function


function mainloop()
	dim do_look = 1
	dim string inp

	while 1
		# show room
		if do_look
			print rooms[croom].description
			if croom == 6  # final room
				return 1
			end if
			# call make_exit_str
			# print "exits:", _ret
			let do_look = 0
		end if
		# get input
		input inp
		if inp == ""
			continue
		end if
		# actions
		if inp == "q" || inp == "quit"
			print "You lie down and rot."
			return
		else
			print "You flail around uselessly."
		end if
	end while
end function


function main()
	dim result
	call buildrooms()
	print "Welcome to the depths."
	let result = mainloop()
	print ""
	if result
		print "You win!"
	else
		print "You are dead."
	end if
	print "Thanks for playing."
end function