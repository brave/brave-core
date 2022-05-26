/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/qr_code/qr_code_generator.h"
#include "base/strings/sys_string_conversions.h"
#include "brave/ios/browser/qr_code_generator/qrcode_generator_service.h"
#include "brave/ios/browser/qr_code_generator/qrcode_models.h"

#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_skia.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// MARK: - Implementation

// MARK: - BraveQRCodeGeneratorOptions
@interface BraveQRCodeGeneratorOptions () {
  std::unique_ptr<qrcode_generator::GenerateQRCodeRequest> request_;
}
@end

@implementation BraveQRCodeGeneratorOptions
- (instancetype)initWithData:(NSString*)data
                shouldRender:(bool)shouldRender
          renderLogoInCenter:(bool)renderLogoInCenter
           renderModuleStyle:(BraveQRCodeGeneratorModuleStyle)renderModuleStyle
          renderLocatorStyle:
              (BraveQRCodeGeneratorLocatorStyle)renderLocatorStyle {
  if ((self = [super init])) {
    request_ = std::make_unique<qrcode_generator::GenerateQRCodeRequest>(
        base::SysNSStringToUTF8(data), shouldRender, renderLogoInCenter,
        static_cast<qrcode_generator::ModuleStyle>(renderModuleStyle),
        static_cast<qrcode_generator::LocatorStyle>(renderLocatorStyle));
  }
  return self;
}

- (void)dealloc {
  request_.reset();
}

- (qrcode_generator::GenerateQRCodeRequest*)nativeRequest {
  return request_.get();
}
@end

// MARK: - BraveQRCodeGeneratorResult

@implementation BraveQRCodeGeneratorResult
- (instancetype)initWithResponse:
    (qrcode_generator::GenerateQRCodeResponse*)response {
  if ((self = [super init])) {
    _errorCode = static_cast<BraveQRCodeGeneratorError>(response->error_code);

    gfx::Image image = gfx::Image(gfx::ImageSkia::CreateFromBitmap(
        response->bitmap, [[UIScreen mainScreen] scale]));
    _image = image.IsEmpty() ? [[UIImage alloc] init] : image.ToUIImage();

    _data = [NSData dataWithBytes:response->data.data()
                           length:response->data.size()];

    _dataSize =
        CGSizeMake(response->data_size.width(), response->data_size.height());
  }
  return self;
}
@end

// MARK: - BraveQRCodeGenerator

@implementation BraveQRCodeGenerator
- (BraveQRCodeGeneratorResult*)generateQRCode:
    (BraveQRCodeGeneratorOptions*)request {
  auto response = qrcode_generator::QRCodeGeneratorService().generateQRCode(
      [request nativeRequest]);
  return [[BraveQRCodeGeneratorResult alloc] initWithResponse:response.get()];
}
@end
