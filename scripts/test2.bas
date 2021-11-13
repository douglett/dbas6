type Shape
	dim string name
	dim int length
end type

function push(Shape@ s)
end function

function main()
	dim Shape sh[2]
	let sh[1].name = "hello world"

	print "here", sh[1].name

	call push(1)
end function