type test_t
	dim a
	dim string s
end type
type test2_t
	dim test_t test
	dim string ss
end type

dim test_t   test
dim test2_t  test2

function main()
	let test.a = 123
	let test.s = "hello"
	let test2.ss = "fart"
	let test2.test.a = 1234

	print test.a, test.s, test2.ss, test2.test.a
end function