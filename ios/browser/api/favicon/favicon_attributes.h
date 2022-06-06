/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_FAVICON_FAVICON_ATTRIBUTES_H_
#define BRAVE_IOS_BROWSER_API_FAVICON_FAVICON_ATTRIBUTES_H_

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
NS_SWIFT_NAME(FaviconLoader.Attributes)
/// Attributes of a favicon. A favicon is represented either with an image or
/// with a fallback monogram of a given color and background color.
@interface BraveFaviconAttributes : NSObject

/// Favicon image. Can be nil. If it is nil, monogram string and color are
/// guaranteed to be not nil.
@property(nonatomic, readonly, nullable) UIImage* faviconImage;
/// Favicon monogram. Only available when there is no image.
@property(nonatomic, readonly, copy, nullable) NSString* monogramString;
/// Favicon monogram color. Only available when there is no image.
@property(nonatomic, readonly, nullable) UIColor* textColor;
/// Favicon monogram background color. Only available when there is no image.
@property(nonatomic, readonly, nullable) UIColor* backgroundColor;
/// Whether the background color is the default one.Only available when there is
/// no image.
@property(nonatomic, readonly) BOOL defaultBackgroundColor;
/// Whether the attributes are using the default image.
@property(nonatomic, readonly) BOOL usesDefaultImage;

+ (nullable instancetype)attributesWithImage:(UIImage*)image
    NS_SWIFT_NAME(init(image:));
+ (nullable instancetype)attributesWithMonogram:(NSString*)monogram
                                      textColor:(UIColor*)textColor
                                backgroundColor:(UIColor*)backgroundColor
                         defaultBackgroundColor:(BOOL)defaultBackgroundColor
    __attribute__((swift_name("init(monogram:textColor:backgroundColor:"
                              "defaultBackgroundColor:)")));

/// Returns attributes with a placeholder favicon image and no monogram.
+ (nullable instancetype)attributesWithDefaultImage;

- (instancetype)init NS_UNAVAILABLE;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_FAVICON_FAVICON_ATTRIBUTES_H_
