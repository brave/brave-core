/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "brave/ios/browser/svg/svg_image.h"

#include "skia/ext/font_utils.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkFontMgr.h"
#include "third_party/skia/include/core/SkRefCnt.h"
#include "third_party/skia/include/core/SkStream.h"
#include "third_party/skia/modules/svg/include/SkSVGDOM.h"
#include "third_party/skia/modules/svg/include/SkSVGRenderContext.h"
#include "third_party/skia/modules/svg/include/SkSVGSVG.h"
#include "ui/gfx/image/image.h"

#include "third_party/skia/include/core/SkSurface.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace SVGImage {
std::unique_ptr<SkCanvas> CreateCanvas(int width, int height) {
  SkBitmap bitmap;

  if (!bitmap.tryAllocPixels(
          SkImageInfo::MakeN32(width, height, kPremul_SkAlphaType))) {
    return nullptr;
  }

  bitmap.eraseARGB(0, 0, 0, 0);
  return std::make_unique<SkCanvas>(bitmap);
}

SkBitmap BitmapFromCanvas(SkCanvas* canvas) {
  SkBitmap bitmap;

  if (!bitmap.tryAllocPixels(canvas->imageInfo())) {
    return bitmap;
  }

  if (!canvas->readPixels(bitmap, 0, 0)) {
    bitmap.reset();
  }
  return bitmap;
}

SkMatrix ComputeScaleMatrix(std::size_t image_width,
                            std::size_t image_height,
                            std::size_t bounds_width,
                            std::size_t bounds_height) {
  return SkMatrix::RectToRect(
      SkRect::MakeIWH(image_width, image_height),
      SkRect::MakeIWH(bounds_width, bounds_height),
      SkMatrix::kCenter_ScaleToFit);  // fPreserveAspectRatio
}

SkBitmap MakeFromData(const NSData* data,
                      std::size_t width,
                      std::size_t height) {
  if ([data length] == 0) {
    return SkBitmap();
  }

  // No need for `SkMemoryStream::MakeCopy` because the stream deallocates
  std::unique_ptr<SkStream> stream =
      SkMemoryStream::MakeDirect([data bytes], [data length]);
  if (!stream) {
    return SkBitmap();
  }

  sk_sp<SkSVGDOM> document =
      SkSVGDOM::Builder().setFontManager(skia::DefaultFontMgr()).make(*stream);
  if (!document || !document->getRoot()) {
    return SkBitmap();
  }

  SkSVGSVG* root_svg_element = document->getRoot();
  root_svg_element->setPreserveAspectRatio(SkSVGPreserveAspectRatio());

  SkSize size =
      root_svg_element->intrinsicSize(SkSVGLengthContext(SkSize::Make(0, 0)));

  if (!size.isEmpty()) {
    document->setContainerSize(size);
  } else {
    const SkRect* viewBox = root_svg_element->getViewBox().getMaybeNull();
    if (viewBox) {
      size = SkSize::Make(viewBox->width(), viewBox->height());
      document->setContainerSize(size);
    } else {
      size = SkSize::Make(SkIntToScalar(width), SkIntToScalar(height));
      document->setContainerSize(size);
    }
  }

  // Same as:
  // sk_sp<SkSurface> surface = SkSurface::MakeRasterN32Premul(width, height);
  // surface->getCanvas();
  std::unique_ptr<SkCanvas> canvas = CreateCanvas(size.width(), size.height());
  if (!canvas) {
    return SkBitmap();
  }

  // Used if scaling the SVG:
  // canvas->concat(ComputeScaleMatrix(size.width(), size.height(), width,
  // height));

  document->render(canvas.get());
  return BitmapFromCanvas(canvas.get());
}
}  // namespace SVGImage
