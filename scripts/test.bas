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
	if a == 1
		print "hello world!"
	else
		print "hello village!"
	end if
	print "hello world 123 &", "poop", aa, 123 + 1 + 10 * 1 - 3
end function