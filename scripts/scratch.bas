type test_t
	dim a
	dim string[] s
end type
type test2_t
	dim test_t[] tests
	dim string   ss
end type

dim test_t     test
dim test2_t    test2

function main()
	push test.s, "fart"
	push test.s, "butt"
	print test.s[0], test.s[1]
	# pop test.s
	# pop test.s

	push test2.tests, test
	push test2.tests, test
	print test2.tests[0].s[1]
end function