#
#  (c) Copyright 2001 -- Anders Torger
#
#  This program is open source. For license terms, see the LICENSE file.
#
################################################################################
#
# This code is an assembler version of the function convolver_convolve_add.
# The C implementation is found elsewhere.
#
# The code uses x87 instructions, compatible with most Intel compatible
# processors. It only supports single precision (32 bit floats).
#
# NOTE: this is a naive version, which is *slow*, GCC can (almost) produce
# faster code than this.
#
# note about comments: b0 == b[0...3], b1 ==  b[4...7]
#
.globl convolver_x87_convolve_add
	.type	convolver_x87_convolve_add,@function
convolver_x87_convolve_add:
	pushl %ebp		# set up stack and save registers
	movl %esp, %ebp
	subl $20, %esp
	pushl %esi
	pushl %edi

	movl 8(%ebp), %esi	# load input pointer, b[]
	movl 12(%ebp), %eax	# load coeffs pointer, c[]
	movl 16(%ebp), %edi	# load output pointer, d[]
	movl 20(%ebp), %ecx	# init loop counter	

	# pre loop setup

	flds (%esi)		# d1s = d[0] + b[0] * c[0]
	fmuls (%eax)
	flds (%edi)
	faddp %st, %st(1)
	fstps -4(%ebp)		# -4(%ebp): d1s

	flds 16(%esi)		# d2s = d[4] + b[4] * c[4]
	fmuls 16(%eax)
	flds 16(%edi)
	faddp %st, %st(1)
	fstps -8(%ebp)		# -8(%ebp): d2s	

	# the loop

	.align 16		# 16 byte loop entry alignment
	.loop_convolve_add:

	flds (%esi)		# d[n+0] += b[n+0] * c[n+0] - b[n+4] * c[n+4]
	fmuls (%eax)
	flds 16(%esi)
	fmuls 16(%eax)
	fsubp %st, %st(1)
	flds (%edi)
	faddp %st, %st(1)
	fstps (%edi)

	flds 4(%esi)		# d[n+1] += b[n+1] * c[n+1] - b[n+5] * c[n+5]
	fmuls 4(%eax)
	flds 20(%esi)
	fmuls 20(%eax)
	fsubp %st, %st(1)
	flds 4(%edi)
	faddp %st, %st(1)
	fstps 4(%edi)

	flds 8(%esi)		# d[n+2] += b[n+2] * c[n+2] - b[n+6] * c[n+6]
	fmuls 8(%eax)
	flds 24(%esi)
	fmuls 24(%eax)
	fsubp %st, %st(1)
	flds 8(%edi)
	faddp %st, %st(1)
	fstps 8(%edi)

	flds 12(%esi)		# d[n+3] += b[n+3] * c[n+3] - b[n+7] * c[n+7]
	fmuls 12(%eax)
	flds 28(%esi)
	fmuls 28(%eax)
	fsubp %st, %st(1)
	flds 12(%edi)
	faddp %st, %st(1)
	fstps 12(%edi)

	flds (%esi)		# d[n+4] += b[n+0] * c[n+4] + b[n+4] * c[n+0]
	fmuls 16(%eax)
	flds 16(%esi)
	fmuls (%eax)
	faddp %st, %st(1)
	flds 16(%edi)
	faddp %st, %st(1)
	fstps 16(%edi)

	flds 4(%esi)		# d[n+5] += b[n+1] * c[n+5] + b[n+5] * c[n+1]
	fmuls 20(%eax)
	flds 20(%esi)
	fmuls 4(%eax)
	faddp %st, %st(1)
	flds 20(%edi)
	faddp %st, %st(1)
	fstps 20(%edi)

	flds 8(%esi)		# d[n+6] += b[n+2] * c[n+6] + b[n+6] * c[n+2]
	fmuls 24(%eax)
	flds 24(%esi)
	fmuls 8(%eax)
	faddp %st, %st(1)
	flds 24(%edi)
	faddp %st, %st(1)
	fstps 24(%edi)

	flds 12(%esi)		# d[n+7] += b[n+3] * c[n+7] + b[n+7] * c[n+3]
	fmuls 28(%eax)
	flds 28(%esi)
	fmuls 12(%eax)
	faddp %st, %st(1)
	flds 28(%edi)
	faddp %st, %st(1)
	fstps 28(%edi)

	addl $32, %esi
	addl $32, %eax
	addl $32, %edi
	decl %ecx		# decrement loop counter
	jnz .loop_convolve_add	# if result of above != 0, goto loop start

	# post loop patch

	movl 16(%ebp), %edi
	movl -4(%ebp), %eax
	movl -8(%ebp), %ecx
	movl %eax, (%edi)       # d[0] = d1s
	movl %ecx, 16(%edi)     # d[4] = d2s

	movl -24(%ebp),%esi	# restore registers and return
	movl -28(%ebp),%edi
	leave
	ret
