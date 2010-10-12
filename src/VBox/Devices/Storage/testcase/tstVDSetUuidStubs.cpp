/* $Id: tstVDSetUuidStubs.cpp $ */
/** @file
 * dummy stubs for RuntimeR3
 */

/*
 * Copyright (C) 2010 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#include <iprt/assert.h>

#ifndef RT_OS_WINDOWS
RTDECL(bool) RTAssertShouldPanic(void)
{
    return true;
}
#endif
