// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

.macro LEAF_ENTRY Name, Section
        .thumb_func
        .global C_FUNC(\Name)
        .type \Name, %function
C_FUNC(\Name):
        .fnstart
.endm

.macro NESTED_ENTRY Name, Section, Handler
        LEAF_ENTRY \Name, \Section
        .ifnc \Handler, NoHandler
        .personality C_FUNC(\Handler)
        .endif
.endm

.macro LEAF_ENTRY_ARM Name, Section
        .arm
        .global C_FUNC(\Name)
        .type \Name, %function
C_FUNC(\Name):
        .fnstart
.endm

.macro NESTED_ENTRY_ARM Name, Section, Handler
        LEAF_ENTRY_ARM \Name, \Section
        .ifnc \Handler, NoHandler
        .personality C_FUNC(\Handler)
        .endif
.endm

.macro NESTED_END Name, Section
        LEAF_END \Name, \Section
.endm

.macro LEAF_END_ARM Name, Section
        .size \Name, .-\Name
        .fnend
        .thumb
.endm

.macro NESTED_END_ARM Name, Section
        LEAF_END_ARM \Name, \Section
.endm

.macro NESTED_END_MARKED Name, Section
        LEAF_END_MARKED \Name, \Section
.endm

.macro PATCH_LABEL Name
        .thumb_func
        .global C_FUNC(\Name)
C_FUNC(\Name):
.endm

.macro LEAF_END Name, Section
        .size \Name, .-\Name
        .fnend
.endm

.macro LEAF_END_MARKED Name, Section
        .thumb_func
        .global C_FUNC(\Name\()_End)
C_FUNC(\Name\()_End):
        LEAF_END \Name, \Section
.endm

.macro PREPARE_EXTERNAL_VAR Name, HelperReg
        ldr \HelperReg, [pc, #C_FUNC(\Name)@GOTPCREL]
.endm

.macro push_nonvol_reg Register
        push \Register
        .save \Register
.endm

.macro pop_nonvol_reg Register
        pop \Register
.endm

.macro vpush_nonvol_reg Register
        vpush \Register
        .vsave \Register
.endm

.macro vpop_nonvol_reg Register
        vpop \Register
.endm

.macro alloc_stack Size
        sub sp, sp, (\Size)
        .pad #(\Size)
.endm

.macro free_stack Size
        add sp, sp, (\Size)
.endm

.macro POP_CALLEE_SAVED_REGISTERS
#if __ARM_ARCH_ISA_THUMB == 1
        pop_nonvol_reg "{r4-r7}"
        pop_nonvol_reg "{lr}"
#else
        pop_nonvol_reg "{r4-r11, lr}"
#endif
.endm

.macro PUSH_CALLEE_SAVED_REGISTERS
#if __ARM_ARCH_ISA_THUMB == 1
        push_nonvol_reg "{lr}"
        push_nonvol_reg "{r4-r7}"
#else
        push_nonvol_reg "{r4-r11, lr}"
#endif
.endm

.macro push_register Reg
        push \Reg
.endm

.macro push_argument_register Reg
        push_register \Reg
.endm

.macro PUSH_ARGUMENT_REGISTERS
        push {r0-r3}
        .save {r0-r3}
.endm

.macro pop_register Reg
        pop \Reg
.endm

.macro pop_argument_register Reg
        pop_register \Reg
.endm

.macro POP_ARGUMENT_REGISTERS
        pop {r0-r3}
.endm

// Stack layout:
//
// (stack parameters)
// ...
// ArgumentRegisters::r3
// ArgumentRegisters::r2
// ArgumentRegisters::r1
// ArgumentRegisters::r0
// CalleeSavedRegisters::lr
// CalleeSavedRegisters::r11
// CalleeSavedRegisters::r10
// CalleeSavedRegisters::r9
// CalleeSavedRegisters::r8
// CalleeSavedRegisters::r7    <- r7
// CalleeSavedRegisters::r6
// CalleeSavedRegisters::r5
// CalleeSavedRegisters::r4    <- __PWTB_StackAlloc, __PWTB_TransitionBlock
// padding to align float save area
// d7
// d6
// d5
// d4
// d3
// d2
// d1
// d0                          <- __PWTB_FloatArgumentRegisters
.macro PROLOG_WITH_TRANSITION_BLOCK extraLocals = 0, saveFpArgs = 1, pushArgRegs = 0

        __PWTB_FloatArgumentRegisters = \extraLocals
        __PWTB_SaveFPArgs = \saveFpArgs

        .if (__PWTB_SaveFPArgs == 1)
                .if ((__PWTB_FloatArgumentRegisters % 8) != 0)
                __PWTB_FloatArgumentRegisters = __PWTB_FloatArgumentRegisters + 4
                .endif

                __PWTB_TransitionBlock = __PWTB_FloatArgumentRegisters + 8 * 8 + 4 // 8 floating point registers + padding
        .else
                .if ((__PWTB_FloatArgumentRegisters % 8) == 0)
                __PWTB_FloatArgumentRegisters = __PWTB_FloatArgumentRegisters + 4
                .endif

                __PWTB_TransitionBlock = __PWTB_FloatArgumentRegisters
        .endif

        __PWTB_StackAlloc = __PWTB_TransitionBlock

        .ifnc \pushArgRegs, DoNotPushArgRegs
                PUSH_ARGUMENT_REGISTERS
        .endif

        PUSH_CALLEE_SAVED_REGISTERS
        PROLOG_STACK_SAVE_OFFSET r7, #12
        // let r7 point the saved r7 in the stack (clang FP style)

        alloc_stack     __PWTB_StackAlloc

        .if (__PWTB_SaveFPArgs == 1)
                add r6, sp, #(__PWTB_FloatArgumentRegisters)
                vstm r6, {d0-d7}
        .endif

        CHECK_STACK_ALIGNMENT
        
        END_PROLOGUE

.endm

.macro EPILOG_WITH_TRANSITION_BLOCK_RETURN

        free_stack __PWTB_StackAlloc
        POP_CALLEE_SAVED_REGISTERS
        free_stack 16
        bx lr

.endm

.macro EPILOG_WITH_TRANSITION_BLOCK_TAILCALL

        .if (__PWTB_SaveFPArgs == 1)
                add r6, sp, #(__PWTB_FloatArgumentRegisters)
                vldm r6, {d0-d7}
        .endif

        free_stack __PWTB_StackAlloc
        POP_CALLEE_SAVED_REGISTERS
        POP_ARGUMENT_REGISTERS

.endm

.macro EMIT_BREAKPOINT
#ifdef __thumb__
	.inst.w 0xde01
#else
	.inst 0xe7f001f0
#endif
.endm

.macro EMIT_BREAKPOINT_ARM
	.inst 0xe7f001f0
.endm

.macro PROLOG_PUSH RegList
        push_nonvol_reg "\RegList"
.endm

.macro PROLOG_VPUSH RegList
        vpush_nonvol_reg "\RegList"
.endm

.macro PROLOG_STACK_SAVE Register
        .setfp \Register, sp
        mov \Register, sp
.endm

.macro PROLOG_STACK_SAVE_OFFSET Register, Offset
        .setfp \Register, sp, \Offset
        add \Register, sp, \Offset
.endm

.macro EPILOG_STACK_FREE Size
        add sp, sp, \Size
.endm

.macro EPILOG_STACK_RESTORE Register
        mov sp, \Register
.endm

.macro EPILOG_STACK_RESTORE_OFFSET Register, Offset
        sub \Register, \Offset
        mov sp, \Register
.endm

.macro EPILOG_BRANCH Target
        b \Target
.endm

.macro EPILOG_BRANCH_REG reg
        bx \reg
.endm

.macro EPILOG_POP RegList
        pop_nonvol_reg "\RegList"
.endm

.macro EPILOG_VPOP RegList
        vpop_nonvol_reg "\RegList"
.endm

//-----------------------------------------------------------------------------
// Macro used to check (in debug builds only) whether the stack is 64-bit aligned (a requirement before calling
// out into C++/OS code). Invoke this directly after your prolog (if the stack frame size is fixed) or directly
// before a call (if you have a frame pointer and a dynamic stack). A breakpoint will be invoked if the stack
// is misaligned.
//
.macro CHECK_STACK_ALIGNMENT

#ifdef _DEBUG
        push {r0, r1}
        add r0, sp, #4
        movs r1, #128
        ands r0, r1
        cmp r0, #0
        pop {r0, r1}
        beq 0f
        EMIT_BREAKPOINT
0:
#endif
.endm

.macro CHECK_STACK_ALIGNMENT_ARM

#ifdef _DEBUG
        push {r0, r1}
        add r0, sp, #4
        movs r1, #128
        ands r0, r1
        cmp r0, #0
        pop {r0, r1}
        beq 0f
        EMIT_BREAKPOINT_ARM
0:
#endif
.endm


.macro cbz_impl Reg, Location
        cmp \Reg, #0
        beq \Location
.endm

#define cbz cbz_impl

.macro cbnz_impl Reg, Location
        cmp \Reg, #0
        bne \Location
.endm

#define cbnz cbnz_impl

#ifdef __ARM_ARCH_6__
.macro dmb_impl
        PUSH_ARGUMENT_REGISTERS
        PUSH_CALLEE_SAVED_REGISTERS
        PROLOG_VPUSH "{d0-d3}"
        mov      r2, #0
        mcr      p15,0,r2,c7,c10,4
        EPILOG_VPOP "{d0-d3}"
        POP_CALLEE_SAVED_REGISTERS
        POP_ARGUMENT_REGISTERS
.endm
#define dmb dmb_impl

.macro movw_impl Reg, value
#ifdef __thumb__
        movs \Reg, \value
#else
        mov \Reg, \value
#endif
.endm

#define movw movw_impl

#if __ARM_ARCH_ISA_THUMB == 1
#define mov movs
#define add adds
#endif

#endif
