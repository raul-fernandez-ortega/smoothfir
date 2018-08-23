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
# The code uses SSE2 instructions operating on 64 bit floating point numbers.
# These instructions are found in Pentium 4 processors.
# Naturally, the code only supports double precision (64 bit floats).
#
# note about comments: b0 == b[0...3], b1 ==  b[4...7]
#
.globl convolver_sse2_convolve_add
	.type	convolver_sse2_convolve_add,@function
convolver_sse2_convolve_add:
	pushl %ebp		# set up stack and save registers
	movl %esp, %ebp
	subl $32, %esp
	pushl %esi
	pushl %edi

	movl 8(%ebp), %esi	# load input pointer, b[]
	movl 12(%ebp), %eax	# load coeffs pointer, c[]
	movl 16(%ebp), %edi	# load output pointer, d[]
	movl 20(%ebp), %ecx	# init loop counter	

	# pre loop setup

	fldl (%esi)		# d1s = d[0] + b[0] * c[0]
	fmull (%eax)
	fldl (%edi)
	faddp %st, %st(1)
	fstpl -8(%ebp)		# -8(%ebp): d1s

	fldl 32(%esi)		# d2s = d[4] + b[4] * c[4]
	fmull 32(%eax)
	fldl 32(%edi)
	faddp %st, %st(1)
	fstpl -16(%ebp)		# -16(%ebp): d2s	

	prefetchnta 128(%esi)
	movapd (%esi), %xmm0	# xmm0: = ba0
	movapd 32(%esi), %xmm1	# xmm1: = ba1
	prefetchnta 128(%eax)
	movapd (%eax), %xmm2	# xmm2: = ca0
	movapd 32(%eax), %xmm3	# xmm3: = ca1
	movapd %xmm0, %xmm4	# xmm4: = ba0
	mulpd %xmm2, %xmm0	# xmm0: ba0 *= ca0
	prefetchnta 128(%edi)
	movapd (%edi), %xmm6	# xmm6: = da0
	movapd 32(%edi), %xmm7	# xmm7: = da1
	mulpd %xmm3, %xmm4	# xmm4: ba0 *= ca1
	movapd %xmm1, %xmm5	# xmm5: = ba1
	mulpd %xmm1, %xmm3	# xmm3: ca1 *= ba1
	addpd %xmm0, %xmm6	# xmm6: da0 += ba0 * ca0
	mulpd %xmm2, %xmm5	# xmm5: ba1 *= ca0
	addpd %xmm4, %xmm7	# xmm7: da1 += ba0 * ca1

	movapd 16(%esi), %xmm0	# xmm0: = bb0
	subpd %xmm3, %xmm6	# xmm6: da0 -= ca1 * ba1
	movapd 48(%esi), %xmm1	# xmm1: = bb1
	movapd 16(%eax), %xmm2	# xmm2: = cb0
	addpd %xmm5, %xmm7	# xmm7: da1 += ba1 * ca0
	movapd 48(%eax), %xmm3	# xmm3: = cb1
	movapd %xmm0, %xmm4	# xmm4: = bb0
	mulpd %xmm2, %xmm0	# xmm0: bb0 *= cb0
	movapd %xmm6, (%edi)	# xmm6: store da0
	movapd %xmm7, 32(%edi)	# xmm7: store da1
	mulpd %xmm3, %xmm4	# xmm4: bb0 *= cb1
	movapd %xmm1, %xmm5	# xmm5: = bb1
	movapd 16(%edi), %xmm6	# xmm6: = db0
	movapd 48(%edi), %xmm7	# xmm7: = db1
	mulpd %xmm2, %xmm5	# xmm5: bb1 *= cb0
	addpd %xmm0, %xmm6	# xmm6: db0 += bb0 * cb0
	mulpd %xmm1, %xmm3	# xmm3: cb1 *= bb1
	addpd %xmm4, %xmm7	# xmm7: db1 += bb0 * cb1

	movapd 64(%esi), %xmm0	# xmm0: = ba0
	movapd 96(%esi), %xmm1	# xmm1: = ba1
	subpd %xmm3, %xmm6	# xmm6: previous db0 -= cb1 * bb1
	movapd 64(%eax), %xmm2	# xmm2: = ca0
	addpd %xmm5, %xmm7	# xmm7: previous db1 += bb1 * cb0
	movapd 96(%eax), %xmm3	# xmm3: = ca1
	movapd %xmm0, %xmm4	# xmm4: = ba0
	movapd %xmm6, 16(%edi)	# xmm6: store previous db0
	movapd %xmm7, 48(%edi)	# xmm7: store previous db1
	addl $64, %edi		# output pointer ++
	mulpd %xmm2, %xmm0	# xmm0: ba0 *= ca0
	movapd (%edi), %xmm6	# xmm6: = da0
	movapd 32(%edi), %xmm7	# xmm7: = da1
	mulpd %xmm3, %xmm4	# xmm4: ba0 *= ca1
	movapd %xmm1, %xmm5	# xmm5: = ba1
	mulpd %xmm1, %xmm3	# xmm3: ca1 *= ba1
	addpd %xmm0, %xmm6	# xmm6: da0 += ba0 * ca0
	mulpd %xmm2, %xmm5	# xmm5: ba1 *= ca0
	addpd %xmm4, %xmm7	# xmm7: da1 += ba0 * ca1

	movapd 80(%esi), %xmm0	# xmm0: = bb0
	subpd %xmm3, %xmm6	# xmm6: da0 -= ca1 * ba1
	movapd 112(%esi), %xmm1	# xmm1: = bb1
	movapd 80(%eax), %xmm2	# xmm2: = cb0
	addpd %xmm5, %xmm7	# xmm7: da1 += ba1 * ca0
	movapd 112(%eax), %xmm3	# xmm3: = cb1
	movapd %xmm0, %xmm4	# xmm4: = bb0
	mulpd %xmm2, %xmm0	# xmm0: bb0 *= cb0
	movapd %xmm6, (%edi)	# xmm6: store da0
	movapd %xmm7, 32(%edi)	# xmm7: store da1
	mulpd %xmm3, %xmm4	# xmm4: bb0 *= cb1
	movapd %xmm1, %xmm5	# xmm5: = bb1
	movapd 16(%edi), %xmm6	# xmm6: = db0
	addl $128, %eax		# coeffs pointer ++
	movapd 48(%edi), %xmm7	# xmm7: = db1
	mulpd %xmm2, %xmm5	# xmm5: bb1 *= cb0
	addl $128, %esi		# input pointer ++
	addpd %xmm0, %xmm6	# xmm6: db0 += bb0 * cb0
	mulpd %xmm1, %xmm3	# xmm3: cb1 *= bb1
	addpd %xmm4, %xmm7	# xmm7: db1 += bb0 * cb1

	decl %ecx		# decrement loop counter

	# the loop

	.align 16		# 16 byte loop entry alignment
	.loop_convolve_add:

	prefetchnta 128(%esi)
	movapd (%esi), %xmm0	# xmm0: = ba0
	movapd 32(%esi), %xmm1	# xmm1: = ba1
	subpd %xmm3, %xmm6	# xmm6: previous db0 -= cb1 * bb1
	prefetchnta 128(%eax)
	movapd (%eax), %xmm2	# xmm2: = ca0
	addpd %xmm5, %xmm7	# xmm7: previous db1 += bb1 * cb0
	movapd 32(%eax), %xmm3	# xmm3: = ca1
	movapd %xmm0, %xmm4	# xmm4: = ba0
	prefetchnta 128(%edi)
	movapd %xmm6, 16(%edi)	# xmm6: store previous db0
	movapd %xmm7, 48(%edi)	# xmm7: store previous db1
	addl $64, %edi		# output pointer ++
	mulpd %xmm2, %xmm0	# xmm0: ba0 *= ca0
	movapd (%edi), %xmm6	# xmm6: = da0
	movapd 32(%edi), %xmm7	# xmm7: = da1
	mulpd %xmm3, %xmm4	# xmm4: ba0 *= ca1
	movapd %xmm1, %xmm5	# xmm5: = ba1
	mulpd %xmm1, %xmm3	# xmm3: ca1 *= ba1
	addpd %xmm0, %xmm6	# xmm6: da0 += ba0 * ca0
	mulpd %xmm2, %xmm5	# xmm5: ba1 *= ca0
	addpd %xmm4, %xmm7	# xmm7: da1 += ba0 * ca1

	movapd 16(%esi), %xmm0	# xmm0: = bb0
	subpd %xmm3, %xmm6	# xmm6: da0 -= ca1 * ba1
	movapd 48(%esi), %xmm1	# xmm1: = bb1
	movapd 16(%eax), %xmm2	# xmm2: = cb0
	addpd %xmm5, %xmm7	# xmm7: da1 += ba1 * ca0
	movapd 48(%eax), %xmm3	# xmm3: = cb1
	movapd %xmm0, %xmm4	# xmm4: = bb0
	mulpd %xmm2, %xmm0	# xmm0: bb0 *= cb0
	movapd %xmm6, (%edi)	# xmm6: store da0
	movapd %xmm7, 32(%edi)	# xmm7: store da1
	mulpd %xmm3, %xmm4	# xmm4: bb0 *= cb1
	movapd %xmm1, %xmm5	# xmm5: = bb1
	movapd 16(%edi), %xmm6	# xmm6: = db0
	movapd 48(%edi), %xmm7	# xmm7: = db1
	mulpd %xmm2, %xmm5	# xmm5: bb1 *= cb0
	addpd %xmm0, %xmm6	# xmm6: db0 += bb0 * cb0
	mulpd %xmm1, %xmm3	# xmm3: cb1 *= bb1
	addpd %xmm4, %xmm7	# xmm7: db1 += bb0 * cb1

	movapd 64(%esi), %xmm0	# xmm0: = ba0
	movapd 96(%esi), %xmm1	# xmm1: = ba1
	subpd %xmm3, %xmm6	# xmm6: previous db0 -= cb1 * bb1
	movapd 64(%eax), %xmm2	# xmm2: = ca0
	addpd %xmm5, %xmm7	# xmm7: previous db1 += bb1 * cb0
	movapd 96(%eax), %xmm3	# xmm3: = ca1
	movapd %xmm0, %xmm4	# xmm4: = ba0
	movapd %xmm6, 16(%edi)	# xmm6: store previous db0
	movapd %xmm7, 48(%edi)	# xmm7: store previous db1
	addl $64, %edi		# output pointer ++
	mulpd %xmm2, %xmm0	# xmm0: ba0 *= ca0
	movapd (%edi), %xmm6	# xmm6: = da0
	movapd 32(%edi), %xmm7	# xmm7: = da1
	mulpd %xmm3, %xmm4	# xmm4: ba0 *= ca1
	movapd %xmm1, %xmm5	# xmm5: = ba1
	mulpd %xmm1, %xmm3	# xmm3: ca1 *= ba1
	addpd %xmm0, %xmm6	# xmm6: da0 += ba0 * ca0
	mulpd %xmm2, %xmm5	# xmm5: ba1 *= ca0
	addpd %xmm4, %xmm7	# xmm7: da1 += ba0 * ca1

	movapd 80(%esi), %xmm0	# xmm0: = bb0
	subpd %xmm3, %xmm6	# xmm6: da0 -= ca1 * ba1
	movapd 112(%esi), %xmm1	# xmm1: = bb1
	movapd 80(%eax), %xmm2	# xmm2: = cb0
	addpd %xmm5, %xmm7	# xmm7: da1 += ba1 * ca0
	movapd 112(%eax), %xmm3	# xmm3: = cb1
	movapd %xmm0, %xmm4	# xmm4: = bb0
	mulpd %xmm2, %xmm0	# xmm0: bb0 *= cb0
	movapd %xmm6, (%edi)	# xmm6: store da0
	movapd %xmm7, 32(%edi)	# xmm7: store da1
	mulpd %xmm3, %xmm4	# xmm4: bb0 *= cb1
	movapd %xmm1, %xmm5	# xmm5: = bb1
	movapd 16(%edi), %xmm6	# xmm6: = db0
	addl $128, %eax		# coeffs pointer ++
	movapd 48(%edi), %xmm7	# xmm7: = db1
	mulpd %xmm2, %xmm5	# xmm5: bb1 *= cb0
	addl $128, %esi		# input pointer ++
	addpd %xmm0, %xmm6	# xmm6: db0 += bb0 * cb0
	mulpd %xmm1, %xmm3	# xmm3: cb1 *= bb1
	addpd %xmm4, %xmm7	# xmm7: db1 += bb0 * cb1

	decl %ecx		# decrement loop counter
	jnz .loop_convolve_add	# if result of above != 0, goto loop start

	# post loop patch

	subpd %xmm3, %xmm6	# xmm6: db0 -= cb1 * bb1
	addpd %xmm5, %xmm7	# xmm7: db1 += bb1 * cb0

	movl 16(%ebp), %esi
	movdqu -16(%ebp), %xmm0
	movhpd %xmm0, (%esi)	# d[0] = d1s
	movlpd %xmm0, 32(%esi)	# d[4] = d2s
	movapd %xmm6, 16(%edi)	# xmm6: store db0
	movapd %xmm7, 48(%edi)	# xmm7: store db1

	movl -36(%ebp),%esi
	movl -40(%ebp),%edi

	leave
	ret
