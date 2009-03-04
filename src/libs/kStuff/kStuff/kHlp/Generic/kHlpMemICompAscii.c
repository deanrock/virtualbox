/* $Id: kHlpMemICompAscii.c 2 2007-11-16 16:07:14Z bird $ */
/** @file
 * kHlpString - kHlpMemICompAscii.
 */

/*
 * Copyright (c) 2006-2007 knut st. osmundsen <bird-kStuff-spam@anduin.net>
 *
 * This file is part of kStuff.
 *
 * kStuff is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * In addition to the permissions in the GNU Lesser General Public
 * License, you are granted unlimited permission to link the compiled
 * version of this file into combinations with other programs, and to
 * distribute those combinations without any restriction coming from
 * the use of this file.
 *
 * kStuff is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with kStuff; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA
 */

/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include <k/kHlpString.h>


KHLP_DECL(int) kHlpMemICompAscii(const void *pv1, const void *pv2, KSIZE cb)
{
    union
    {
        const void *pv;
        const KU8 *pb;
        const KUPTR *pu;
    } u1, u2;

    u1.pv = pv1;
    u2.pv = pv2;

    if (cb >= 32)
    {
        while (cb > sizeof(KUPTR))
        {
            if (*u1.pu != *u2.pu)
                break; /* hand it on to the byte-by-byte routine. */
            u1.pu++;
            u2.pu++;
            cb -= sizeof(KUPTR);
        }
    }

    while (cb-- > 0)
    {
        if (u1.pb != u2.pb)
        {
            KU8 ch1 = *u1.pb;
            KU8 ch2 = *u2.pb;
            if (ch1 <= 'Z' && ch1 >= 'A')
                ch1 += 'a' - 'A';
            if (ch2 <= 'Z' && ch2 >= 'A')
                ch2 += 'a' - 'A';
            if (ch1 != ch2)
                return ch1 > ch2 ? 1 : -1;
        }
        u1.pb++;
        u2.pb++;
    }

    return 0;
}
