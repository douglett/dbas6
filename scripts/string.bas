dim string a = "hello"
dim string b = "world"

function square(int a)
	return a * a
end function

function dup(string s)
	let s = s + s
end function

function dup2(string s, string d)
	let s = s + d + d
end function

function main()
	dim string hi = a + " " + b
	dim string bb = b
	let hi = "[[" + hi + "]]" 
	print a; b; hi; 1+1*2+4/2

	if a == "hel" + "lo"
		print "a is hel + lo"
	end if

	print "square(3) is " square(3)
	call dup(bb)
	print "dup result: " bb
	let bb = b
	call dup2(bb, "poop")
	print "dup2 result: " bb
end function