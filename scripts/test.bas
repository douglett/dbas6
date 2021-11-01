type test
	dim a
	# a comment
	dim string b
end type


# comment 2
type poop
	dim a
	dim b
	dim integer c
	# dim test t
end type

dim aa
dim a
# dim string b

function f(int a)
	dim c
	print "hello world 123 &", "poop", aa, 123 + 1 + 10 * 1 - 3
end function

function main()
	print "hello world"
	let a = 5+5
	if a == 10
		print "a is 10!"
	else
		print "a is NOT 10!"
	end if
end function