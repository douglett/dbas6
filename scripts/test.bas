# type test
# 	dim a
# 	# a comment
# 	dim string b
# end type

dim a = 1 * 2 * 4 + 4 * 3

function square(int a)
	return a * a
end function

function main()
	dim b = 500
	dim c = 0
	let c = 5+5-3-2+1
	print "a, b, c is:", a, b, c

	call square(2)
	
# 	# if a == 10
# 	# 	print "a is 10!"
# 	# else
# 	# 	print "a is NOT 10!"
# 	# end if
# 	# call f(20)
# 	# let a = 10 + square(4)
# 	# print "inline call result:", a

# 	# let b = "blah" + "_" + "flob"
# 	# # let b = "ass"
# 	# print "[" b "]"

# 	# print "fart" + "butt"; "poop"
end function