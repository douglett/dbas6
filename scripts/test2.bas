function test(string s)
	print "here", s
end function

function main()
	dim string s = "fart"
	call test(s)
	call test("poop")
end function