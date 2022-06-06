/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "brave/ios/browser/api/favicon/favicon_attributes.h"
#include "ios/chrome/common/ui/favicon/favicon_attributes.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// MARK: - Implementation

@implementation BraveFaviconAttributes
- (instancetype)initWithAttributes:(FaviconAttributes*)attributes {
  if ((self = [super init])) {
    _faviconImage = [attributes.faviconImage copy];
    _monogramString = [attributes.monogramString copy];
    _textColor = [attributes.textColor copy];
    _backgroundColor = [attributes.backgroundColor copy];
    _defaultBackgroundColor = attributes.defaultBackgroundColor;
    _usesDefaultImage = attributes.usesDefaultImage;
  }
  return self;
}

+ (instancetype)attributesWithImage:(UIImage*)image {
  FaviconAttributes* attributes = [FaviconAttributes attributesWithImage:image];
  return attributes
             ? [[BraveFaviconAttributes alloc] initWithAttributes:attributes]
             : nil;
}

+ (instancetype)attributesWithMonogram:(NSString*)monogram
                             textColor:(UIColor*)textColor
                       backgroundColor:(UIColor*)backgroundColor
                defaultBackgroundColor:(BOOL)defaultBackgroundColor {
  FaviconAttributes* attributes =
      [FaviconAttributes attributesWithMonogram:monogram
                                      textColor:textColor
                                backgroundColor:backgroundColor
                         defaultBackgroundColor:defaultBackgroundColor];
  return attributes
             ? [[BraveFaviconAttributes alloc] initWithAttributes:attributes]
             : nil;
}

+ (instancetype)attributesWithDefaultImage {
  FaviconAttributes* attributes =
      [FaviconAttributes attributesWithDefaultImage];
  return attributes
             ? [[BraveFaviconAttributes alloc] initWithAttributes:attributes]
             : nil;
}
@end
