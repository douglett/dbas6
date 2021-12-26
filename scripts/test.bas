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
	dim b = 500, c = 0, i
	let c = 5+5-3-2+1
	print "a, b, c is:", a, b, c

	call square(2)
	let a = square(3)
	let a = 10 + square(4)
	print "inline call result:", a
	
	if a == 26
		print "a is 26!"
	else
		print "a is NOT 26!"
	end if

	while true
		let i = i + 1
		print i
		if i >= 10
			break
		end if
		continue
		print "nope lol"
	end while
end function