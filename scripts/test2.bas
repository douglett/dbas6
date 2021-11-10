type Shape
	dim int length
	dim string name
end type

type Circle
	dim Shape sh
end type

dim a

function main()
	dim string a
	dim Shape sh
	dim Circle c
	dim b

	let sh.length = 100
	let b = sh.length

	print b, sh.length

	let c.sh.name = "fart"
end function