type Shape
	dim string name
	dim int length
end type

function main()
	dim Shape sh[2]
	let sh[1].name = "hello world"

	print "here", sh[1].name

	print "len1:"; len(sh); len(sh[1].name)
	redim sh, 3
	print "len2:"; len(sh)

	# call push(sh, sh[1])
end function