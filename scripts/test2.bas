# dim string a

function main()
	dim string a
	dim i
	let a = "butt" + "poop" + "y"
	print "here", a

	if a == "buttpoopy"
		print "Yes"
	else
		print "No"
	end if

	while i < 10
		let i = i + 1
		if i == 9
			break
		else if i == 3
			continue
		end if
		print "i is " i
	end while
end function