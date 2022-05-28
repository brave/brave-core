/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_QR_CODE_QR_CODE_GENERATOR_H_
#define BRAVE_IOS_BROWSER_API_QR_CODE_QR_CODE_GENERATOR_H_

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSInteger, BraveQRCodeGeneratorLocatorStyle) {
  BraveQRCodeGeneratorLocatorStyleDefaultSquare,
  BraveQRCodeGeneratorLocatorStyleRounded
} NS_SWIFT_NAME(QRCodeGenerator.LocatorStyle);

typedef NS_ENUM(NSInteger, BraveQRCodeGeneratorModuleStyle) {
  BraveQRCodeGeneratorModuleStyleDefaultSquares,
  BraveQRCodeGeneratorModuleStyleCircles
} NS_SWIFT_NAME(QRCodeGenerator.LocatorStyle);

typedef NS_ENUM(NSInteger, BraveQRCodeGeneratorError) {
  BraveQRCodeGeneratorErrorNone,
  BraveQRCodeGeneratorErrorInputTooLong,
  BraveQRCodeGeneratorErrorUnknown,
} NS_SWIFT_NAME(QRCodeGenerator.Error);

OBJC_EXPORT
NS_SWIFT_NAME(QRCodeGenerator.Options)
@interface BraveQRCodeGeneratorOptions : NSObject
- (instancetype)initWithData:(NSString*)data
                shouldRender:(bool)shouldRender
          renderLogoInCenter:(bool)renderLogoInCenter
           renderModuleStyle:(BraveQRCodeGeneratorModuleStyle)renderModuleStyle
          renderLocatorStyle:
              (BraveQRCodeGeneratorLocatorStyle)renderLocatorStyle;
@end

// Structure for returning QR Code image data.
OBJC_EXPORT
NS_SWIFT_NAME(QRCodeGenerator.Result)
@interface BraveQRCodeGeneratorResult : NSObject
// Return code stating success or failure.
@property(nonatomic, readonly) BraveQRCodeGeneratorError errorCode;
// Image data for generated QR code. May be null on error, or if rendering
// was not requested.
@property(nullable, nonatomic, readonly) UIImage* image;
// QR Code data.
@property(nonatomic, readonly) NSData* data;
// 2-D size of |data| in elements. Note |image| will be upscaled, so this
// does not represent the returned image size.
@property(nonatomic, readonly) CGSize dataSize;

- (instancetype)init NS_UNAVAILABLE;
@end

OBJC_EXPORT
NS_SWIFT_NAME(QRCodeGenerator)
@interface BraveQRCodeGenerator : NSObject
- (BraveQRCodeGeneratorResult*)generateQRCode:
    (BraveQRCodeGeneratorOptions*)request;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_QR_CODE_QR_CODE_GENERATOR_H_
