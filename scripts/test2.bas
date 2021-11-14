type Shape
	dim string name
	dim int length
end type

function push(Shape s[])
	print "here2", s[1].name
end function

function main()
	dim Shape sh[2]
	let sh[1].name = "hello world"

	print "here", sh[1].name

	call push(sh)
end function