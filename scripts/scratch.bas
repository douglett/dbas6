type test_t
	dim a
	dim string s
end type
# type test2_t
# 	dim test_t test
# end type


dim string[] rooms
dim test_t   test

function init()
	push rooms, "entrance"
	push rooms, "a cave"
	push rooms, "a lake"
end function

function main()
	dim i
	dim string aa = "ass"
	call init()
	for i = 0 to 2
		print i, rooms[i]
	end for

	let aa[1] = 66
	let test.a = 100
	let test.s = "fart"
	print aa, test.a, test.s

end function