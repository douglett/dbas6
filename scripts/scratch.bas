dim string s  = "hello"
dim string[] sa

function main()
	dim int i
	dim string[] sb = sa
	push s, 65
	print "here: [" s "]"
	pop s
	pop s
	print "here2: [" s "], " i
	
	push sa, s
	print "array test:", sa[0]
	push sa, "blahblah"
	print "array test 2:", sa[1]
	let sb = sa
	let sa[0] = "poop"
	print "array test 3:", sa[1+1-2]

	print "array test 3:", sb[1+1-2]

	# pop sa
end function