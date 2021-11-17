DougBasic-6
===========

(dbas-6)

WIP basic (again)


TODO:
-----
- arguments as string expressions explode runtime
	- prevent this and add warning
	- difference between string variable and expression types?
	- look into better string handling
	- difference between string reference and constructor?
- void types for standard library?
- call statements
	- predefined standard library functions
		- special hidden multi-type arguments (any array, array/string, etc)
		- array resize
		- push, pop
		- insert, erase
	- argument syntax? @ / &
- input command
- malloc / memmalloc merge?
- for loops
- function hoisting
- consts
- argument consts
- constexpr at top level dim

DONE:
-----
- single line multi-dim
- dim with initialise (int / string)
- changed dim syntax for arrays from C to Java
	- int a[100] -> int[] a(100)
- call statements
	- variant types
	- implement non-owner pointer types
	- argument type checking
- varpath array / object chains
- varpath arrays
- switch from isarray to type[]
- dim arrays
- auto malloc / free
	- user type automalloc
	- arrmalloc
- varpath
	- cleanup
	- object properties
	- object chains
- proper integer support
- while
	- break
	- continue
	- nested breakout (break 2, continue 2)
- varpath - difference between 'get' and 'set' paths
- strings 
	- string equality (strcmp / strncmp)
	- string addition (strcat)
	- proper string expressions
	- string pointer support
	- string malloc / free
	- basic string expressions in let
- string auto malloc / free
	- circular includes
	- malloc format
	- global malloc block
	- global free block
	- local malloc block
	- local free block
	- runtime malloc
	- local malloc in functions + runtime
- return statement
- runtime calls in expressions
- runtime calls
- call statements
	- argument counting
- order of addition and subtraction!
- running simple programs
- first runtime primatives
- basic expressions
- basic assignment
- first item
