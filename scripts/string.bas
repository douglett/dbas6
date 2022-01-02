dim string a = "hello"
dim string b = "world"

function square(int a)
	return a * a
end function

function strdup(string s)
	let s = s + s
end function

function main()
	dim string hi = a + " " + b
	let hi = "[[" + hi + "]]" 
	print a; b; hi

	if a == "hel" + "lo"
		print "a is hel + lo"
	end if

	print "square(3) is " square(3)
	call strdup(b)
	print "dup result: " b
end function