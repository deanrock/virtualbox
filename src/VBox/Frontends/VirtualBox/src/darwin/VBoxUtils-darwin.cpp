/* $Id: VBoxUtils-darwin.cpp $ */
/** @file
 * Qt GUI - Utility Classes and Functions specific to Darwin.
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
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 USA or visit http://www.sun.com if you need
 * additional information or have any questions.
 */



#include "VBoxUtils.h"
#include "VBoxFrameBuffer.h"
#include <qimage.h>
#include <qpixmap.h>

#include <iprt/assert.h>
#include <iprt/mem.h>


/**
 * Callback for deleting the QImage object when CGImageCreate is done
 * with it (which is probably not until the returned CFGImageRef is released).
 *
 * @param   info        Pointer to the QImage.
 */
static void darwinDataProviderReleaseQImage (void *info, const void *, size_t)
{
    QImage *qimg = (QImage *)info;
    delete qimg;
}

/**
 * Converts a QPixmap to a CGImage.
 *
 * @returns CGImageRef for the new image. (Remember to release it when finished with it.)
 * @param   aPixmap     Pointer to the QPixmap instance to convert.
 */
CGImageRef DarwinQImageToCGImage (const QImage *aImage)
{
    QImage *imageCopy = new QImage (*aImage);
    /** @todo this code assumes 32-bit image input, the lazy bird convert image to 32-bit method is anything but optimal... */
    if (imageCopy->depth() != 32)
        *imageCopy = imageCopy->convertDepth (32);
    Assert (!imageCopy->isNull());

    CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
    CGDataProviderRef dp = CGDataProviderCreateWithData (imageCopy, aImage->bits(), aImage->numBytes(), darwinDataProviderReleaseQImage);

    CGBitmapInfo bmpInfo = imageCopy->hasAlphaBuffer() ? kCGImageAlphaFirst : kCGImageAlphaNoneSkipFirst;
    bmpInfo |= kCGBitmapByteOrder32Host;
    CGImageRef ir = CGImageCreate (imageCopy->width(), imageCopy->height(), 8, 32, imageCopy->bytesPerLine(), cs,
                                   bmpInfo, dp, 0 /*decode */, 0 /* shouldInterpolate */,
                                   kCGRenderingIntentDefault);
    CGColorSpaceRelease (cs);
    CGDataProviderRelease (dp);

    Assert (ir);
    return ir;
}

/**
 * Loads an image using Qt and converts it to a CGImage.
 *
 * @returns CGImageRef for the new image. (Remember to release it when finished with it.)
 * @param   aSource     The source name.
 */
CGImageRef DarwinQImageFromMimeSourceToCGImage (const char *aSource)
{
    QImage qim = QImage::fromMimeSource (aSource);
    Assert (!qim.isNull());
    return DarwinQImageToCGImage (&qim);
}

/**
 * Converts a QPixmap to a CGImage.
 *
 * @returns CGImageRef for the new image. (Remember to release it when finished with it.)
 * @param   aPixmap     Pointer to the QPixmap instance to convert.
 */
CGImageRef DarwinQPixmapToCGImage (const QPixmap *aPixmap)
{
    QImage qimg = aPixmap->convertToImage();
    Assert (!qimg.isNull());
    return DarwinQImageToCGImage (&qimg);
}

/**
 * Loads an image using Qt and converts it to a CGImage.
 *
 * @returns CGImageRef for the new image. (Remember to release it when finished with it.)
 * @param   aSource     The source name.
 */
CGImageRef DarwinQPixmapFromMimeSourceToCGImage (const char *aSource)
{
    QPixmap qpm = QPixmap::fromMimeSource (aSource);
    Assert (!qpm.isNull());
    return DarwinQPixmapToCGImage (&qpm);
}

/**
 * Creates a dock badge image.
 *
 * The badge will be placed on the right hand size and vertically centered
 * after having been scaled up to 32x32.
 *
 * @returns CGImageRef for the new image. (Remember to release it when finished with it.)
 * @param   aSource     The source name.
 */
CGImageRef DarwinCreateDockBadge (const char *aSource)
{
    /* Create a transparent image in size 128x128.
     * This is unnecessary complicated in qt3. */
    QImage transImage (128, 128, 32);
    transImage.fill (qRgba (0, 0, 0, 0));
    transImage.setAlphaBuffer (true);
    QPixmap back (transImage);

    /* load the badge */
    QPixmap badge = QPixmap::fromMimeSource (aSource);
    Assert (!badge.isNull());

    /* resize it and copy it onto the background. */
    if (badge.width() < 32)
        badge = badge.convertToImage().smoothScale (32, 32);
    copyBlt (&back, (back.width() - badge.width()) / 2.0, (back.height() - badge.height()) / 2.0,
             &badge, 0, 0,
             badge.width(), badge.height());

    /* Convert it to a CGImage. */
    return ::DarwinQPixmapToCGImage (&back);
}

/**
 * Updates the dock preview image.
 *
 * This method is a callback that updates the 128x128 preview image of the VM window in the dock.
 *
 * @param   aVMImage   the vm screen as a CGImageRef
 * @param   aOverlayImage   an optional icon overlay image to add at the bottom right of the icon
 * @param   aStateImage   an optional state overlay image to add at the center of the icon
 */
void DarwinUpdateDockPreview (CGImageRef aVMImage, CGImageRef aOverlayImage, CGImageRef aStateImage /*= NULL*/)
{
    Assert (aVMImage);

    CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
    Assert (cs);

    /* Calc the size of the dock icon image and fit it into 128x128 */
    int targetWidth = 128;
    int targetHeight = 128;
    int scaledWidth;
    int scaledHeight;
    float aspect = static_cast <float> (CGImageGetWidth (aVMImage)) / CGImageGetHeight (aVMImage);
    if (aspect > 1.0)
    {
        scaledWidth = targetWidth;
        scaledHeight = targetHeight / aspect;
    }
    else
    {
        scaledWidth = targetWidth * aspect;
        scaledHeight = targetHeight;
    }
    CGRect iconRect = CGRectMake ((targetWidth - scaledWidth) / 2.0,
                                  (targetHeight - scaledHeight) / 2.0,
                                  scaledWidth, scaledHeight);
    /* Create the context to draw on */
    CGContextRef context = BeginCGContextForApplicationDockTile();
    Assert (context);
    /* Clear the background */
    CGContextSetBlendMode (context, kCGBlendModeNormal);
    CGContextClearRect (context, CGRectMake (0, 0, 128, 128));
    /* rounded corners */
//        CGContextSetLineJoin (context, kCGLineJoinRound);
//        CGContextSetShadow (context, CGSizeMake (10, 5), 1);
//        CGContextSetAllowsAntialiasing (context, true);
    /* some little boarder */
    iconRect = CGRectInset (iconRect, 1, 1);
    /* gray stroke */
    CGContextSetRGBStrokeColor (context, 225.0/255.0, 218.0/255.0, 211.0/255.0, 1);
    iconRect = CGRectInset (iconRect, 6, 6);
    CGContextStrokeRectWithWidth (context, iconRect, 12);
    iconRect = CGRectInset (iconRect, 5, 5);
    /* black stroke */
    CGContextSetRGBStrokeColor (context, 0.0, 0.0, 0.0, 1.0);
    CGContextStrokeRectWithWidth (context, iconRect, 2);
    /* vm content */
    iconRect = CGRectInset (iconRect, 1, 1);
    CGContextDrawImage (context, iconRect, aVMImage);
    /* the state image at center */
    if (aStateImage)
    {
        CGRect stateRect = CGRectMake ((targetWidth - CGImageGetWidth (aStateImage)) / 2.0,
                                       (targetHeight - CGImageGetHeight (aStateImage)) / 2.0,
                                       CGImageGetWidth (aStateImage),
                                       CGImageGetHeight (aStateImage));
        CGContextDrawImage (context, stateRect, aStateImage);
    }
    /* the overlay image at bottom/right */
    if (aOverlayImage)
    {

        CGRect overlayRect = CGRectMake (targetWidth - CGImageGetWidth (aOverlayImage),
                                         0,
                                         CGImageGetWidth (aOverlayImage),
                                         CGImageGetHeight (aOverlayImage));
        CGContextDrawImage (context, overlayRect, aOverlayImage);
    }
    /* This flush updates the dock icon */
    CGContextFlush (context);
    EndCGContextForApplicationDockTile (context);

    CGColorSpaceRelease (cs);
}

/**
 * Updates the dock preview image.
 *
 * This method is a callback that updates the 128x128 preview image of the VM window in the dock.
 *
 * @param   aFrameBuffer    The guest frame buffer.
 * @param   aOverlayImage   an optional icon overlay image to add at the bottom right of the icon
 */
void DarwinUpdateDockPreview (VBoxFrameBuffer *aFrameBuffer, CGImageRef aOverlayImage)
{
    CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
    Assert (cs);
    /* Create the image copy of the framebuffer */
    CGDataProviderRef dp = CGDataProviderCreateWithData (aFrameBuffer, aFrameBuffer->address(), aFrameBuffer->bitsPerPixel() / 8 * aFrameBuffer->width() * aFrameBuffer->height(), NULL);
    Assert (dp);
    CGImageRef ir = CGImageCreate (aFrameBuffer->width(), aFrameBuffer->height(), 8, 32, aFrameBuffer->bytesPerLine(), cs,
                                   kCGImageAlphaNoneSkipFirst | kCGBitmapByteOrder32Host, dp, 0, false,
                                   kCGRenderingIntentDefault);
    /* Update the dock preview icon */
    ::DarwinUpdateDockPreview (ir, aOverlayImage);
    /* Release the temp data and image */
    CGDataProviderRelease (dp);
    CGImageRelease (ir);
    CGColorSpaceRelease (cs);
}

OSStatus DarwinRegionHandler (EventHandlerCallRef aInHandlerCallRef, EventRef aInEvent, void *aInUserData)
{
    NOREF (aInHandlerCallRef);

    OSStatus status = eventNotHandledErr;

    switch (GetEventKind (aInEvent))
    {
        case kEventWindowGetRegion:
        {
            WindowRegionCode code;
            RgnHandle rgn;

            /* which region code is being queried? */
            GetEventParameter (aInEvent, kEventParamWindowRegionCode, typeWindowRegionCode, NULL, sizeof (code), NULL, &code);

            /* if it is the opaque region code then set the region to Empty and return noErr to stop the propagation */
            if (code == kWindowOpaqueRgn)
            {
                GetEventParameter (aInEvent, kEventParamRgnHandle, typeQDRgnHandle, NULL, sizeof (rgn), NULL, &rgn);
                SetEmptyRgn (rgn);
                status = noErr;
            }
            /* if the content of the whole window is queried return a copy of our saved region. */
            else if (code == (kWindowStructureRgn))// || kWindowGlobalPortRgn || kWindowUpdateRgn))
            {
                GetEventParameter (aInEvent, kEventParamRgnHandle, typeQDRgnHandle, NULL, sizeof (rgn), NULL, &rgn);
                QRegion *pRegion = static_cast <QRegion*> (aInUserData);
                if (!pRegion->isNull() && pRegion)
                {
                    CopyRgn (pRegion->handle(), rgn);
                    status = noErr;
                }
            }
            break;
        }
    }

    return status;
}

/* Event debugging stuff. Borrowed from the Knuts Qt patch. */
#ifdef DEBUG

# define MY_CASE(a) case a: return #a
const char * DarwinDebugEventName (UInt32 ekind)
{
    switch (ekind)
    {
        MY_CASE(kEventWindowUpdate                );
        MY_CASE(kEventWindowDrawContent           );
        MY_CASE(kEventWindowActivated             );
        MY_CASE(kEventWindowDeactivated           );
        MY_CASE(kEventWindowHandleActivate        );
        MY_CASE(kEventWindowHandleDeactivate      );
        MY_CASE(kEventWindowGetClickActivation    );
        MY_CASE(kEventWindowGetClickModality      );
        MY_CASE(kEventWindowShowing               );
        MY_CASE(kEventWindowHiding                );
        MY_CASE(kEventWindowShown                 );
        MY_CASE(kEventWindowHidden                );
        MY_CASE(kEventWindowCollapsing            );
        MY_CASE(kEventWindowCollapsed             );
        MY_CASE(kEventWindowExpanding             );
        MY_CASE(kEventWindowExpanded              );
        MY_CASE(kEventWindowZoomed                );
        MY_CASE(kEventWindowBoundsChanging        );
        MY_CASE(kEventWindowBoundsChanged         );
        MY_CASE(kEventWindowResizeStarted         );
        MY_CASE(kEventWindowResizeCompleted       );
        MY_CASE(kEventWindowDragStarted           );
        MY_CASE(kEventWindowDragCompleted         );
        MY_CASE(kEventWindowClosed                );
        MY_CASE(kEventWindowTransitionStarted     );
        MY_CASE(kEventWindowTransitionCompleted   );
        MY_CASE(kEventWindowClickDragRgn          );
        MY_CASE(kEventWindowClickResizeRgn        );
        MY_CASE(kEventWindowClickCollapseRgn      );
        MY_CASE(kEventWindowClickCloseRgn         );
        MY_CASE(kEventWindowClickZoomRgn          );
        MY_CASE(kEventWindowClickContentRgn       );
        MY_CASE(kEventWindowClickProxyIconRgn     );
        MY_CASE(kEventWindowClickToolbarButtonRgn );
        MY_CASE(kEventWindowClickStructureRgn     );
        MY_CASE(kEventWindowCursorChange          );
        MY_CASE(kEventWindowCollapse              );
        MY_CASE(kEventWindowCollapseAll           );
        MY_CASE(kEventWindowExpand                );
        MY_CASE(kEventWindowExpandAll             );
        MY_CASE(kEventWindowClose                 );
        MY_CASE(kEventWindowCloseAll              );
        MY_CASE(kEventWindowZoom                  );
        MY_CASE(kEventWindowZoomAll               );
        MY_CASE(kEventWindowContextualMenuSelect  );
        MY_CASE(kEventWindowPathSelect            );
        MY_CASE(kEventWindowGetIdealSize          );
        MY_CASE(kEventWindowGetMinimumSize        );
        MY_CASE(kEventWindowGetMaximumSize        );
        MY_CASE(kEventWindowConstrain             );
        MY_CASE(kEventWindowHandleContentClick    );
        MY_CASE(kEventWindowGetDockTileMenu       );
        MY_CASE(kEventWindowProxyBeginDrag        );
        MY_CASE(kEventWindowProxyEndDrag          );
        MY_CASE(kEventWindowToolbarSwitchMode     );
        MY_CASE(kEventWindowFocusAcquired         );
        MY_CASE(kEventWindowFocusRelinquish       );
        MY_CASE(kEventWindowFocusContent          );
        MY_CASE(kEventWindowFocusToolbar          );
        MY_CASE(kEventWindowFocusDrawer           );
        MY_CASE(kEventWindowSheetOpening          );
        MY_CASE(kEventWindowSheetOpened           );
        MY_CASE(kEventWindowSheetClosing          );
        MY_CASE(kEventWindowSheetClosed           );
        MY_CASE(kEventWindowDrawerOpening         );
        MY_CASE(kEventWindowDrawerOpened          );
        MY_CASE(kEventWindowDrawerClosing         );
        MY_CASE(kEventWindowDrawerClosed          );
        MY_CASE(kEventWindowDrawFrame             );
        MY_CASE(kEventWindowDrawPart              );
        MY_CASE(kEventWindowGetRegion             );
        MY_CASE(kEventWindowHitTest               );
        MY_CASE(kEventWindowInit                  );
        MY_CASE(kEventWindowDispose               );
        MY_CASE(kEventWindowDragHilite            );
        MY_CASE(kEventWindowModified              );
        MY_CASE(kEventWindowSetupProxyDragImage   );
        MY_CASE(kEventWindowStateChanged          );
        MY_CASE(kEventWindowMeasureTitle          );
        MY_CASE(kEventWindowDrawGrowBox           );
        MY_CASE(kEventWindowGetGrowImageRegion    );
        MY_CASE(kEventWindowPaint                 );
    }
    static char s_sz[64];
    sprintf(s_sz, "kind=%d", ekind);
    return s_sz;
}
# undef MY_CASE

/* Convert a class into the 4 char code defined in
 * 'Developer/Headers/CFMCarbon/CarbonEvents.h' to 
 * identify the event. */
const char * DarwinDebugClassName (UInt32 eclass)
{
    char *pclass = (char*)&eclass;
    static char s_sz[11];
    sprintf(s_sz, "class=%c%c%c%c", pclass[3], 
                                    pclass[2], 
                                    pclass[1], 
                                    pclass[0]);
    return s_sz;
}

void DarwinDebugPrintEvent (const char *psz, EventRef event)
{
  if (!event)
      return;
  UInt32 ekind = GetEventKind (event), eclass = GetEventClass (event);
  if (eclass == kEventClassWindow)
  {
      switch (ekind)
      {
          case kEventWindowDrawContent:
          case kEventWindowUpdate:
          case kEventWindowBoundsChanged:
              break;
          default:
          {
              WindowRef wid = NULL;
              GetEventParameter(event, kEventParamDirectObject, typeWindowRef, NULL, sizeof(WindowRef), NULL, &wid);
              QWidget *widget = QWidget::find((WId)wid);
              printf("%d %s: (%s) %#x win=%p wid=%p (%s)\n", time(NULL), psz, DarwinDebugClassName (eclass), ekind, wid, widget, DarwinDebugEventName (ekind));
              break;
          }
      }
  }
  else if (eclass == kEventClassCommand)
  {
      WindowRef wid = NULL;
      GetEventParameter(event, kEventParamDirectObject, typeWindowRef, NULL, sizeof(WindowRef), NULL, &wid);
      QWidget *widget = QWidget::find((WId)wid);
      const char *name = "Unknown";
      switch (ekind)
      {
          case kEventCommandProcess:
              name = "kEventCommandProcess";
              break;
          case kEventCommandUpdateStatus:
              name = "kEventCommandUpdateStatus";
              break;
      }
      printf("%d %s: (%s) %#x win=%p wid=%p (%s)\n", time(NULL), psz, DarwinDebugClassName (eclass), ekind, wid, widget, name);
  }
  else if (eclass == kEventClassKeyboard)
      printf("%d %s: %#x(%s) %#x (kEventClassKeyboard)\n", time(NULL), psz, eclass, DarwinDebugClassName (eclass), ekind);

  else
      printf("%d %s: %#x(%s) %#x\n", time(NULL), psz, eclass, DarwinDebugClassName (eclass), ekind);
}

#endif /* DEBUG */