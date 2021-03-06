/* $Id: DnDPath.cpp $ */
/** @file
 * DnD: Path handling.
 */

/*
 * Copyright (C) 2014 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

/******************************************************************************
 *   Header Files                                                             *
 ******************************************************************************/

#include <iprt/path.h>
#include <iprt/string.h>

#include <VBox/GuestHost/DragAndDrop.h>

/**
 * Sanitizes the file name component so that unsupported characters
 * will be replaced by an underscore ("_").
 *
 * @return  IPRT status code.
 * @param   pszPath             Path to sanitize.
 * @param   cbPath              Size (in bytes) of path to sanitize.
 */
int DnDPathSanitizeFilename(char *pszPath, size_t cbPath)
{
    int rc = VINF_SUCCESS;
#ifdef RT_OS_WINDOWS
    /* Filter out characters not allowed on Windows platforms, put in by
       RTTimeSpecToString(). */
    /** @todo Use something like RTPathSanitize() when available. Later. */
    RTUNICP aCpSet[] =
        { ' ', ' ', '(', ')', '-', '.', '0', '9', 'A', 'Z', 'a', 'z', '_', '_',
          0xa0, 0xd7af, '\0' };
    ssize_t cReplaced = RTStrPurgeComplementSet(pszPath, aCpSet, '_' /* Replacement */);
    if (cReplaced < 0)
        rc = VERR_INVALID_UTF8_ENCODING;
#endif
    return rc;
}

int DnDPathSanitize(char *pszPath, size_t cbPath)
{
    /** @todo */
    return VINF_SUCCESS;
}

