dim string s  = "hello"
dim string[] sa

function main()
	dim int i
	push s, 65
	print "here: [" s "]"
	pop s
	# pop s, i
	print "here2: [" s "], " i

	push sa, s

	# push lines, "fart"
	# pop lines, s
	# insert lines, 4, s
	# erase lines, 4, s
end function