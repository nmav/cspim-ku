#################################################
#					 	#
#		text segment			#
#						#
#################################################

	.text		
       	.globl __start 
#       	.globl main
__start:
	jal main
	nop
	break
