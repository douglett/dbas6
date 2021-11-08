# dim string a

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

	# call testret()
	
	# if testret() && testret()
	# else if testret() || testret()
	# end if
end function

# function testret()
# 	print "test"
# end function