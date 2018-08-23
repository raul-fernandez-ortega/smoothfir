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
# The code uses SSE instructions, found in Pentium III and Pentium 4 processors.
# It only supports single precision (32 bit floats).
#
# note about comments: b0 == b[0...3], b1 ==  b[4...7]
#
.globl convolver_sse_convolve_add
	.type	convolver_sse_convolve_add,@function
convolver_sse_convolve_add:
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

	prefetchnta 96(%esi)
	movaps (%esi), %xmm0	# xmm0: = b0
	movaps 16(%esi), %xmm1	# xmm1: = b1
	prefetchnta 96(%eax)
	movaps (%eax), %xmm2	# xmm2: = c0
	movaps 16(%eax), %xmm3	# xmm3: = c1
	movaps %xmm0, %xmm4	# xmm4: = b0
	movaps %xmm1, %xmm5	# xmm5: = b1
	mulps %xmm2, %xmm0	# xmm0: b0 *= c0
	mulps %xmm3, %xmm4	# xmm4: b0 *= c1
	prefetchnta 96(%edi)
	movaps (%edi), %xmm6	# xmm6: = d0
	movaps 16(%edi), %xmm7	# xmm7: = d1
	mulps %xmm2, %xmm5	# xmm5: b1 *= c0
	mulps %xmm1, %xmm3	# xmm3: c1 *= b1
	addps %xmm0, %xmm6	# xmm6: d0 += b0 * c0
	addps %xmm4, %xmm7	# xmm7: d1 += b0 * c1
	addl $32, %eax		# coeffs pointer ++
	addl $32, %esi		# input pointer ++
	decl %ecx		# decrement loop counter

	# the loop

	.align 16		# 16 byte loop entry alignment
	.loop_convolve_add:
	prefetchnta 96(%esi)
	movaps (%esi), %xmm0	# xmm0: = b0
	movaps 16(%esi), %xmm1	# xmm1: = b1
	subps %xmm3, %xmm6	# xmm6: previous d0 -= c1 * b1
	addps %xmm5, %xmm7	# xmm7: previous d1 += b1 * c0
	prefetchnta 96(%eax)
	movaps (%eax), %xmm2	# xmm2: = c0
	movaps 16(%eax), %xmm3	# xmm3: = c1
	movaps %xmm6, (%edi)	# store d0
	movaps %xmm7, 16(%edi)	# store d1
	addl $32, %eax		# coeffs pointer ++
	addl $32, %edi		# output pointer ++
	movaps %xmm0, %xmm4	# xmm4: = b0
	movaps %xmm1, %xmm5	# xmm5: = b1
	mulps %xmm2, %xmm0	# xmm0: b0 *= c0
	mulps %xmm3, %xmm4	# xmm4: b0 *= c1
	prefetcht2 96(%edi)
	movaps (%edi), %xmm6	# xmm6: = d0
	movaps 16(%edi), %xmm7	# xmm7: = d1
	mulps %xmm2, %xmm5	# xmm5: b1 *= c0
	mulps %xmm1, %xmm3	# xmm3: c1 *= b1
	addps %xmm0, %xmm6	# xmm6: d0 += b0 * c0
	addps %xmm4, %xmm7	# xmm7: d1 += b0 * c1
	addl $32, %esi		# input pointer ++
	decl %ecx		# decrement loop counter
	jnz .loop_convolve_add	# if result of above != 0, goto loop start

	# post loop patch

	subps %xmm3, %xmm6	# xmm6: d0 -= b1 * c1
	addps %xmm5, %xmm7	# xmm7: d1 += b1 * c0

	movl 16(%ebp), %esi
	movl -4(%ebp), %eax
	movl -8(%ebp), %ecx
	movl %eax, (%esi)       # d[0] = d1s
	movl %ecx, 16(%esi)     # d[4] = d2s
	movaps %xmm6, (%edi)	# store d0
	movaps %xmm7, 16(%edi)	# store d1

	movl -24(%ebp),%esi	# restore registers and return
	movl -28(%ebp),%edi
	leave
	ret
