dim string s = "hello"

function dup(string dest, string s)
	let dest = dest + s + s
end function

function main()
	call dup(s, "balls")
	print "result:"; s
end function