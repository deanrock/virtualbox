/* $Id: kHlpPage-iprt.cpp 8155 2008-04-18 15:16:47Z vboxsync $ */
/** @file
 * kHlpPage - Page Memory Allocation, IPRT based implementation.
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

/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include <k/kHlpAlloc.h>
#include <k/kErrors.h>
#include <iprt/mem.h>
#include <iprt/assert.h>
#include <iprt/err.h>



static unsigned kHlpPageProtToNative(KPROT enmProt)
{
    switch (enmProt)
    {
        case KPROT_NOACCESS:             return RTMEM_PROT_NONE;
        case KPROT_READONLY:             return RTMEM_PROT_READ;
        case KPROT_READWRITE:            return RTMEM_PROT_READ | RTMEM_PROT_WRITE;
        case KPROT_EXECUTE:              return RTMEM_PROT_EXEC;
        case KPROT_EXECUTE_READ:         return RTMEM_PROT_EXEC | RTMEM_PROT_READ;
        case KPROT_EXECUTE_READWRITE:    return RTMEM_PROT_EXEC | RTMEM_PROT_READ | RTMEM_PROT_WRITE;
        default:
            AssertFailed();
            return ~0U;
    }
}


KHLP_DECL(int) kHlpPageAlloc(void **ppv, KSIZE cb, KPROT enmProt, KBOOL fFixed)
{
    AssertReturn(fFixed, KERR_NOT_IMPLEMENTED);

    int rc = VINF_SUCCESS;
    void *pv = RTMemPageAlloc(cb);
    if (pv)
    {
        rc = RTMemProtect(pv, cb, kHlpPageProtToNative(enmProt));
        if (RT_SUCCESS(rc))
            *ppv = pv;
        else
            RTMemPageFree(pv);
    }
    return rc;
}


KHLP_DECL(int) kHlpPageProtect(void *pv, KSIZE cb, KPROT enmProt)
{
    int rc = RTMemProtect(pv, cb, kHlpPageProtToNative(enmProt));
    if (!rc)
        return 0;
    /** @todo convert iprt status code -> kErrors */
    return rc;
}


KHLP_DECL(int) kHlpPageFree(void *pv, KSIZE cb)
{
    RTMemPageFree(pv);
    return 0;
}
