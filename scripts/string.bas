dim string a = "hello"
dim string b = "world"
dim string hi = a + " :: " + b

function main()
	dim g
	# dim string c = "inside string"
	# print "strings:"
	# print "[" a "]"; "[" b "]"; "[" c "]"

	let g = 1 + 2 + 3 + 4
	let hi = "[[" + hi + "]]" 
	print hi; "g", g

	# if a == a
	# 	print "a is a"
	# end if
	# if a != b
	# 	print "a is not b"
	# end if
	# if a == "hello"
	# 	print "a is hello"
	# end if
	if a == "hel" + "lo"
		print "a is hel + lo"
	end if

	# return 2*2+1
	# print "fart"
end function