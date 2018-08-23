#
#  (c) Copyright 2001 -- Anders Torger
#
#  This program is open source. For license terms, see the LICENSE file.
#
################################################################################
#
# This code is an assembler versions of the function convolver_convolve_add.
# The C implementation is found elsewhere.
#
# The code uses 3Dnow instructions, found in AMD K6-2, K6-III and Athlon
# processors. It only supports single precision (32 bit floats).
#
# note about comments: ba0 == b[0...1], bb0 == b[2...3]
#                      ba1 == b[4...5], bb1 == b[6...7]
#
.globl convolver_3dnow_convolve_add
	.type	convolver_3dnow_convolve_add,@function
convolver_3dnow_convolve_add:
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

	femms			# enter mmx/3dnow state

	prefetch 96(%esi)
	movq (%esi), %mm0	# mm0: = ba0
	movq 16(%esi), %mm1	# mm1: = ba1
	prefetch 96(%eax)
	movq (%eax), %mm2	# mm2: = ca0
	movq 16(%eax), %mm3	# mm3: = ca1
	movq %mm0, %mm4		# mm4: = ba0
	pfmul %mm2, %mm0	# mm0: ba0 *= ca0
	prefetchw 96(%edi)
	movq (%edi), %mm6	# mm6: = da0
	movq 16(%edi), %mm7	# mm7: = da1
	pfmul %mm3, %mm4	# mm4: ba0 *= ca1
	movq %mm1, %mm5		# mm5: = ba1
	pfmul %mm1, %mm3	# mm3: ca1 *= ba1
	pfadd %mm0, %mm6	# mm6: da0 += ba0 * ca0
	pfmul %mm2, %mm5	# mm5: ba1 *= ca0
	pfadd %mm4, %mm7	# mm7: da1 += ba0 * ca1

	movq 8(%esi), %mm0	# mm0: = bb0
	pfsub %mm3, %mm6	# mm6: da0 -= ca1 * ba1
	movq 24(%esi), %mm1	# mm1: = bb1
	movq 8(%eax), %mm2	# mm2: = cb0
	pfadd %mm5, %mm7	# mm7: da1 += ba1 * ca0
	movq 24(%eax), %mm3	# mm3: = cb1
	movq %mm0, %mm4		# mm4: = bb0
	pfmul %mm2, %mm0	# mm0: bb0 *= cb0
	movq %mm6, (%edi)	# mm6: store da0
	movq %mm7, 16(%edi)	# mm7: store da1
	pfmul %mm3, %mm4	# mm4: bb0 *= cb1
	movq %mm1, %mm5		# mm5: = bb1
	movq 8(%edi), %mm6	# mm6: = db0
	addl $32, %eax		# coeffs pointer ++
	movq 24(%edi), %mm7	# mm7: = db1
	pfmul %mm2, %mm5	# mm5: bb1 *= cb0
	addl $32, %esi		# input pointer ++
	pfadd %mm0, %mm6	# mm6: db0 += bb0 * cb0
	pfmul %mm1, %mm3	# mm3: cb1 *= bb1
	pfadd %mm4, %mm7	# mm7: db1 += bb0 * cb1
	decl %ecx		# decrement loop counter

	# the loop

	.align 16		# 16 byte loop entry alignment
	.loop_convolve_add:

	prefetch 96(%esi)
	movq (%esi), %mm0	# mm0: = ba0
	movq 16(%esi), %mm1	# mm1: = ba1
	pfsub %mm3, %mm6	# mm6: previous db0 -= cb1 * bb1
	prefetch 96(%eax)
	movq (%eax), %mm2	# mm2: = ca0
	pfadd %mm5, %mm7	# mm7: previous db1 += bb1 * cb0
	movq 16(%eax), %mm3	# mm3: = ca1
	movq %mm0, %mm4		# mm4: = ba0
	prefetchw 96(%edi)
	movq %mm6, 8(%edi)	# mm6: store previous db0
	movq %mm7, 24(%edi)	# mm7: store previous db1
	addl $32, %edi		# output pointer ++
	pfmul %mm2, %mm0	# mm0: ba0 *= ca0
	movq (%edi), %mm6	# mm6: = da0
	movq 16(%edi), %mm7	# mm7: = da1
	pfmul %mm3, %mm4	# mm4: ba0 *= ca1
	movq %mm1, %mm5		# mm5: = ba1
	pfmul %mm1, %mm3	# mm3: ca1 *= ba1
	pfadd %mm0, %mm6	# mm6: da0 += ba0 * ca0
	pfmul %mm2, %mm5	# mm5: ba1 *= ca0
	pfadd %mm4, %mm7	# mm7: da1 += ba0 * ca1

	movq 8(%esi), %mm0	# mm0: = bb0
	pfsub %mm3, %mm6	# mm6: da0 -= ca1 * ba1
	movq 24(%esi), %mm1	# mm1: = bb1
	movq 8(%eax), %mm2	# mm2: = cb0
	pfadd %mm5, %mm7	# mm7: da1 += ba1 * ca0
	movq 24(%eax), %mm3	# mm3: = cb1
	movq %mm0, %mm4		# mm4: = bb0
	pfmul %mm2, %mm0	# mm0: bb0 *= cb0
	movq %mm6, (%edi)	# mm6: store da0
	movq %mm7, 16(%edi)	# mm7: store da1
	pfmul %mm3, %mm4	# mm4: bb0 *= cb1
	movq %mm1, %mm5		# mm5: = bb1
	movq 8(%edi), %mm6	# mm6: = db0
	addl $32, %eax		# coeffs pointer ++
	movq 24(%edi), %mm7	# mm7: = db1
	pfmul %mm2, %mm5	# mm5: bb1 *= cb0
	addl $32, %esi		# input pointer ++
	pfadd %mm0, %mm6	# mm6: db0 += bb0 * cb0
	pfmul %mm1, %mm3	# mm3: cb1 *= bb1
	pfadd %mm4, %mm7	# mm7: db1 += bb0 * cb1

	decl %ecx		# decrement loop counter
	jnz .loop_convolve_add	# if result of above != 0, goto loop start

	# post loop patch

	pfsub %mm3, %mm6	# mm6: db0 -= cb1 * bb1
	pfadd %mm5, %mm7	# mm7: db1 += bb1 * cb0

	movl 16(%ebp), %esi
	movl -4(%ebp), %eax
	movl -8(%ebp), %ecx
	movl %eax, (%esi)       # d[0] = d1s
	movl %ecx, 16(%esi)     # d[4] = d2s
	movq %mm6, 8(%edi)	# mm6: store db0
	movq %mm7, 24(%edi)	# mm7: store db1

	movl -24(%ebp),%esi
	movl -28(%ebp),%edi

	femms			# exit mmx/3dnow state

	leave
	ret
