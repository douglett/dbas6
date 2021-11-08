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
dim string b

function f(int a)
	dim c
	print "func f calling!", "argument:", a
	print "hello world 123 &", "poop", aa, 123 + 1 + 10 * 1 - 3
end function

function square(int a)
	return a * a
end function

function main()
	print "hello world"
	let a = 5+5-3-2+1
	print "a is:", a
	if a == 10
		print "a is 10!"
	else
		print "a is NOT 10!"
	end if
	call f(20)
	let a = 10 + square(4)
	print "inline call result:", a

	let b = "blah" + "_" + "flob"
	# let b = "ass"
	print "[" b "]"

	print "fart" + "butt"; "poop"
end function