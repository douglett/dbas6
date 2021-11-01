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
	if a == 1
		print "a is 1!"
	else
		print "a is NOT 1!"
	end if
end function