dim string s  = "hello"
dim string[] sa

function main()
	dim int i
	push s, 65
	print "here: [" s "]"
	pop s
	pop s
	print "here2: [" s "], " i
	push sa, s
	# pop sa
end function