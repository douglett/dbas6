# dim string a

function test_ret(int n)
	print "test", n
	return 1
end function

function main()
	dim string a
	dim i
	dim j
	let a = "butt" + "poop" + "y"
	print "here", a

	if a == "buttpoopy"
		print "Yes"
	else
		print "No"
	end if

	while i < 3
		let i = i + 1
		while j < 3
			let j = j + 1
			print "i", i; "j", j
			if i == 2 && j == 2
				break 2
			end if
		end while
		let j = 0
	end while

	print "the end"

	# call test_ret(0)

	if test_ret(1) && test_ret(1) && false
	else if test_ret(2) || test_ret(2)
	end if
end function