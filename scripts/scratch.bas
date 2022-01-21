type test_t
	dim a
	dim string s
end type
type test2_t
	dim test_t test
	dim string ss
end type

dim test_t     test
dim test_t[]   arr
dim test2_t    test2
dim test2_t[]  arr2

function main()
	let test.a = 1
	let test.s = "fart"
	
	print "here1"
	push arr, test
	print "here1", arr[0].a, arr[0].s

	let test2.test = test
	push arr2, test2
	print "here2", arr2[0].test.a, arr2[0].test.s
	
	push arr2, test2
	let arr2[0].ss = "poop"
	let arr2[1].ss = "poopy"
	let arr2[1].test.s = "poofart"
	print "here3", arr2[0].ss, arr2[0].test.s, arr2[1].ss, arr2[1].test.s
end function