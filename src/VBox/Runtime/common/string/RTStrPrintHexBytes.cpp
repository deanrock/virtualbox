/* $Id: RTStrPrintHexBytes.cpp 23507 2009-10-02 12:02:02Z vboxsync $ */
/** @file
 * IPRT - RTStrPrintHexBytes.
 */

/*
 * Copyright (C) 2009 Sun Microsystems, Inc.
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL) only, as it comes in the "COPYING.CDDL" file of the
 * VirtualBox OSE distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 *
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 USA or visit http://www.sun.com if you need
 * additional information or have any questions.
 */


/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include "internal/iprt.h"
#include <iprt/string.h>

#include <iprt/assert.h>
#include <iprt/err.h>


RTDECL(int) RTStrPrintHexBytes(char *pszBuf, size_t cchBuf, void const *pv, size_t cb, uint32_t fFlags)
{
    AssertReturn(!fFlags, VERR_INVALID_PARAMETER);
    AssertPtrReturn(pszBuf, VERR_INVALID_POINTER);
    AssertReturn(cb * 2 >= cb, VERR_BUFFER_OVERFLOW);
    AssertReturn(cchBuf >= cb * 2 + 1, VERR_BUFFER_OVERFLOW);
    if (cb)
        AssertPtrReturn(pv, VERR_INVALID_POINTER);

    uint8_t const *pb = (uint8_t const *)pv;
    while (cb-- > 0)
    {
        static char const s_szHexDigits[17] = "0123456789abcdef";
        uint8_t b = *pb++;
        *pszBuf++ = s_szHexDigits[b >> 4];
        *pszBuf++ = s_szHexDigits[b & 0xf];
    }
    *pszBuf = '\0';
    return VINF_SUCCESS;
}
