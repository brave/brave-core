/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_QR_CODE_GENERATOR_QRCODE_GENERATOR_SERVICE_H_
#define BRAVE_IOS_BROWSER_QR_CODE_GENERATOR_QRCODE_GENERATOR_SERVICE_H_

#include <memory>
#include "brave/ios/browser/qr_code_generator/qrcode_models.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkCanvas.h"

namespace qrcode_generator {
class QRCodeGeneratorService {
 public:
  QRCodeGeneratorService();
  QRCodeGeneratorService(const QRCodeGeneratorService&) = delete;
  QRCodeGeneratorService& operator=(const QRCodeGeneratorService&) = delete;

  ~QRCodeGeneratorService();

  std::unique_ptr<GenerateQRCodeResponse> generateQRCode(
      const GenerateQRCodeRequest* request);

 private:
  // Renders dino data into a 1x bitmap, |dino_bitmap_|, owned by the class.
  // This is simpler and faster than repainting it from static source data
  // each time.
  void InitializeDinoBitmap();

  // Draws a dino image at the center of |canvas|.
  // In the common case where drawing at the same scale as QR modules, note that
  // the QR Code versions from the spec all consist of n*n modules, with n odd,
  // while the dino data is w*h for w,h even, so it will be offset.
  void DrawDino(SkCanvas* canvas,
                const SkRect& canvas_bounds,
                const int pixels_per_dino_tile,
                const int dino_border_px,
                const SkPaint& paint_foreground,
                const SkPaint& paint_background);

  // Renders the QR code with pixel information in |data| and render parameters
  // in |request|. Result is stored into |response|.
  // |data| is input data, one element per module, row-major.
  // |data_size| is the dimensions of |data|, in modules. Currently expected to
  //     be square, but function should cope with other shapes.
  // |request| is the mojo service request object to Generate().
  //     It includes rendering style preferences expressed by the client.
  // |response| is the mojo service request object to Generate().
  //     The bitmap will be populated as a response field if requested by the
  //     client.
  void RenderBitmap(const uint8_t* data,
                    const gfx::Size data_size,
                    const GenerateQRCodeRequest* const& request,
                    GenerateQRCodeResponse* response);

  SkBitmap dino_bitmap_;
};

}  // namespace qrcode_generator

#endif  // BRAVE_IOS_BROWSER_QR_CODE_GENERATOR_QRCODE_GENERATOR_SERVICE_H_
