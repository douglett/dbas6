type T
	dim string s
end type

function main()
	dim T t[2]
	let t[1].s = "hello world"

	print "here", t[1].s
end function