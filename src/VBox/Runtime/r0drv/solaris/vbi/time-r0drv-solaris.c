/* $Id: time-r0drv-solaris.c 9149 2008-05-27 09:27:29Z vboxsync $ */
/** @file
 * IPRT - Time, Ring-0 Driver, Solaris.
 */

/*
 * Copyright (C) 2006-2007 Sun Microsystems, Inc.
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
#include "the-solaris-kernel.h"
#define RTTIME_INCL_TIMESPEC

#include <iprt/time.h>


RTDECL(uint64_t) RTTimeNanoTS(void)
{
    return (uint64_t)gethrtime();
}


RTDECL(uint64_t) RTTimeMilliTS(void)
{
    return RTTimeNanoTS() / 1000000;
}


RTDECL(uint64_t) RTTimeSystemNanoTS(void)
{
    return RTTimeNanoTS();
}


RTDECL(uint64_t) RTTimeSystemMilliTS(void)
{
    return RTTimeNanoTS() / 1000000;
}


RTDECL(PRTTIMESPEC) RTTimeNow(PRTTIMESPEC pTime)
{
    return RTTimeSpecSetNano(pTime, vbi_tod());
}
