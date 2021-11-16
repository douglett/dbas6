type Shape
	dim string name
	dim int length
end type

function push(Shape s[], Shape ss)
	print "here2"; s[1].name; ss.name
	# dim len = sizeof(s)
	# redim s[len+1]
	# let s[len] = ss
	# return len+1
end function

# function pop($array s)

function main()
	dim Shape sh[2]
	let sh[1].name = "hello world"

	print "here", sh[1].name

	call push(sh, sh[1])

	print "len1:"; len(sh); len(sh[1].name)
	redim sh, 3
	print "len2:"; len(sh)

	# redim sh, 2
	# push sh, s
	# pop sh, s
	# insert sh, 1, s
	# slice sh, 1, s

end function