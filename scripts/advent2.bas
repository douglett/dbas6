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


function strlen(string str)
	dim i
	len str, i
	return i
end function


function split(string str, string[] arr)
	dim i, res
	dim string s
	dim string[] clear
	let arr = clear  # clear array
	# loop string
	for i = 0 to strlen(str) - 1
		# if word break, push previous word
		if str[i] == 32 || str[i] == 9
			if strlen(s)
				push arr, s
				let s = ""
			end if
		# increment previous word
		else
			push s, str[i]
		end if
	end for
	# add last word if needed
	if strlen(s)
		push arr, s
	end if
	# return items in arr
	len arr, res
	return res
end function


function move(int dir)
	dim i, roomslen
	dim string target, dirname
	# set up directions
	if dir == 0
		let target = rooms[croom].exit_n
		let dirname = "north"
	else if dir == 1
		let target = rooms[croom].exit_e
		let dirname = "east"
	else if dir == 2
		let target = rooms[croom].exit_s
		let dirname = "south"
	else if dir == 3
		let target = rooms[croom].exit_w
		let dirname = "west"
	end if
	# move
	len rooms, roomslen
	for i = 0 to roomslen - 1
		if rooms[i].name == target
			let croom = i
			print "You go " dirname "."
			return 1
		end if
	end for
	# could not move
	print "You can't go " dirname "."
	return 0
end function


function mainloop()
	dim l, do_look = 1
	dim string inp
	dim string[] cmd

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
		let l = split(inp, cmd)
		print "commands:", l
		if l == 0
			continue
		end if
		# actions
		if cmd[0] == "q" || cmd[0] == "quit"
			print "You lie down and rot."
			return
		else if cmd[0] == "l" || cmd[0] == "look"
			print "You look around you."
			let do_look = 1
		else if cmd[0] == "n" || cmd[0] == "north"
			let do_look = move(0)
		else if cmd[0] == "s" || cmd[0] == "south"
			let do_look = move(2)
		else if cmd[0] == "e" || cmd[0] == "east"
			let do_look = move(1)
		else if cmd[0] == "w" || cmd[0] == "west"
			let do_look = move(3)
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