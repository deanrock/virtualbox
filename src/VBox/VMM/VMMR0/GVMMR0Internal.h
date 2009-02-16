/* $Id: GVMMR0Internal.h 14811 2008-11-29 23:48:26Z vboxsync $ */
/** @file
 * GVMM - The Global VM Manager, Internal header.
 */

/*
 * Copyright (C) 2007 Sun Microsystems, Inc.
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 *
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 USA or visit http://www.sun.com if you need
 * additional information or have any questions.
 */

#ifndef ___GVMMR0Internal_h
#define ___GVMMR0Internal_h

#include <iprt/mem.h>


/**
 * The GVMM per VM data.
 */
typedef struct GVMMPERVM
{
    /** The shared VM data structure allocation object (PVMR0). */
    RTR0MEMOBJ          VMMemObj;
    /** The Ring-3 mapping of the shared VM data structure (PVMR3). */
    RTR0MEMOBJ          VMMapObj;
    /** The allocation object for the VM pages. */
    RTR0MEMOBJ          VMPagesMemObj;
    /** The ring-3 mapping of the VM pages. */
    RTR0MEMOBJ          VMPagesMapObj;

    /** The time the halted EMT thread expires.
     * 0 if the EMT thread is blocked here. */
    uint64_t volatile   u64HaltExpire;
    /** The event semaphore the EMT thread is blocking on. */
    RTSEMEVENTMULTI     HaltEventMulti;
    /** The APIC ID of the CPU that EMT was scheduled on the last time we checked. */
    uint8_t             iCpuEmt;

    /** The scheduler statistics. */
    GVMMSTATSSCHED      StatsSched;

    /** Whether the per-VM ring-0 initialization has been performed. */
    bool                fDoneVMMR0Init;
    /** Whether the per-VM ring-0 termination is being or has been performed. */
    bool                fDoneVMMR0Term;
} GVMMPERVM;
/** Pointer to the GVMM per VM data. */
typedef GVMMPERVM *PGVMMPERVM;


#endif
