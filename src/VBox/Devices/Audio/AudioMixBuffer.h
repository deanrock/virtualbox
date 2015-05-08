/* $Id: AudioMixBuffer.h $ */
/** @file
 * VBox audio: TODO
 */

/*
 * Copyright (C) 2014-2015 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef AUDIO_MIXBUF_H
#define AUDIO_MIXBUF_H

#include <iprt/cdefs.h>
#include <VBox/vmm/pdmaudioifs.h>

/** Constructs 32 bit value for given frequency, number of channels, bits per sample and signed bit.
 *  Note: This currently matches 1:1 the VRDE encoding -- this might change in the future, so better don't rely on this fact! */
#define AUDMIXBUF_AUDIO_FMT_MAKE(freq, c, bps, s) ((((s) & 0x1) << 28) + (((bps) & 0xFF) << 20) + (((c) & 0xF) << 16) + ((freq) & 0xFFFF))

/** Decodes frequency (Hz). */
#define AUDMIXBUF_FMT_SAMPLE_FREQ(a) ((a) & 0xFFFF)
/** Decodes number of channels. */
#define AUDMIXBUF_FMT_CHANNELS(a) (((a) >> 16) & 0xF)
/** Decodes signed bit. */
#define AUDMIXBUF_FMT_SIGNED(a) (((a) >> 28) & 0x1)
/** Decodes number of bits per sample. */
#define AUDMIXBUF_FMT_BITS_PER_SAMPLE(a) (((a) >> 20) & 0xFF)
/** Decodes number of bytes per sample. */
#define AUDMIXBUF_FMT_BYTES_PER_SAMPLE(a) ((AUDMIXBUF_AUDIO_FMT_BITS_PER_SAMPLE(a) + 7) / 8)

/** Converts samples to bytes. */
#define AUDIOMIXBUF_S2B(pBuf, samples) ((samples) << (pBuf)->cShift)
/** Converts samples to bytes, respecting the conversion ratio to
 *  a linked buffer. */
#define AUDIOMIXBUF_S2B_RATIO(pBuf, samples) ((((int64_t) samples << 32) / (pBuf)->iFreqRatio) << (pBuf)->cShift)
/** Converts bytes to samples, *not* taking the conversion ratio
 *  into account. */
#define AUDIOMIXBUF_B2S(pBuf, cb)  (cb >> (pBuf)->cShift)
/** Converts number of samples according to the buffer's ratio. */
#define AUDIOMIXBUF_S2S_RATIO(pBuf, samples)  (((int64_t) samples << 32) / (pBuf)->iFreqRatio)


int audioMixBufAcquire(PPDMAUDIOMIXBUF pMixBuf, uint32_t cSamplesToRead, PPDMAUDIOSAMPLE *ppvSamples, uint32_t *pcSamplesRead);
inline uint32_t audioMixBufBytesToSamples(PPDMAUDIOMIXBUF pMixBuf);
void audioMixBufDestroy(PPDMAUDIOMIXBUF pMixBuf);
void audioMixBufFinish(PPDMAUDIOMIXBUF pMixBuf, uint32_t cSamplesToClear);
uint32_t audioMixBufFree(PPDMAUDIOMIXBUF pMixBuf);
uint32_t audioMixBufFreeBytes(PPDMAUDIOMIXBUF pMixBuf);
int audioMixBufInit(PPDMAUDIOMIXBUF pMixBuf, const char *pszName, PPDMPCMPROPS pProps, uint32_t cSamples);
bool audioMixBufIsEmpty(PPDMAUDIOMIXBUF pMixBuf);
int audioMixBufLinkTo(PPDMAUDIOMIXBUF pMixBuf, PPDMAUDIOMIXBUF pParent);
uint32_t audioMixBufMixed(PPDMAUDIOMIXBUF pMixBuf);
int audioMixBufMixToChildren(PPDMAUDIOMIXBUF pMixBuf, uint32_t cSamples, uint32_t *pcProcessed);
int audioMixBufMixToParent(PPDMAUDIOMIXBUF pMixBuf, uint32_t cSamples, uint32_t *pcProcessed);
uint32_t audioMixBufProcessed(PPDMAUDIOMIXBUF pMixBuf);
int audioMixBufReadAt(PPDMAUDIOMIXBUF pMixBuf, uint32_t offSamples, void *pvBuf, uint32_t cbBuf, uint32_t *pcbRead);
int audioMixBufReadAtEx(PPDMAUDIOMIXBUF pMixBuf, PDMAUDIOMIXBUFFMT enmFmt, uint32_t offSamples, void *pvBuf, uint32_t cbBuf, uint32_t *pcbRead);
int audioMixBufReadCirc(PPDMAUDIOMIXBUF pMixBuf, void *pvBuf, uint32_t cbBuf, uint32_t *pcRead);
int audioMixBufReadCircEx(PPDMAUDIOMIXBUF pMixBuf, PDMAUDIOMIXBUFFMT enmFmt, void *pvBuf, uint32_t cbBuf, uint32_t *pcRead);
void audioMixBufReset(PPDMAUDIOMIXBUF pMixBuf);
void audioMixBufSetVolume(PPDMAUDIOMIXBUF pMixBuf, PPDMAUDIOVOLUME pVol);
uint32_t audioMixBufSize(PPDMAUDIOMIXBUF pMixBuf);
uint32_t audioMixBufSizeBytes(PPDMAUDIOMIXBUF pMixBuf);
void audioMixBufUnlink(PPDMAUDIOMIXBUF pMixBuf);
int audioMixBufWriteAt(PPDMAUDIOMIXBUF pMixBuf, uint32_t offSamples, const void *pvBuf, uint32_t cbBuf, uint32_t *pcWritten);
int audioMixBufWriteAtEx(PPDMAUDIOMIXBUF pMixBuf, PDMAUDIOMIXBUFFMT enmFmt, uint32_t offSamples, const void *pvBuf, uint32_t cbBuf, uint32_t *pcWritten);
int audioMixBufWriteCirc(PPDMAUDIOMIXBUF pMixBuf, const void *pvBuf, uint32_t cbBuf, uint32_t *pcWritten);
int audioMixBufWriteCircEx(PPDMAUDIOMIXBUF pMixBuf, PDMAUDIOMIXBUFFMT enmFmt, const void *pvBuf, uint32_t cbBuf, uint32_t *pcWritten);

#endif /* AUDIO_MIXBUF_H */
