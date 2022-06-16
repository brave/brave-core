/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "brave/ios/browser/api/favicon/favicon_view.h"
#import "brave/ios/browser/api/favicon/favicon_attributes.h"
#include "ios/chrome/common/ui/favicon/favicon_attributes.h"
#include "ios/chrome/common/ui/favicon/favicon_view.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// MARK: - Implementation

@interface BraveFaviconAttributes (Private)
@property(nonatomic, strong, readonly) FaviconAttributes* attributes;
@end

@interface BraveFaviconView ()
@property(nonatomic, strong, readonly) FaviconView* view;
@end

@implementation BraveFaviconView
- (instancetype)init {
  if ((self = [super init])) {
    _view = [[FaviconView alloc] init];

    [self addSubview:_view];
    [_view setTranslatesAutoresizingMaskIntoConstraints:NO];

    [NSLayoutConstraint activateConstraints:@[
      [_view.leadingAnchor constraintEqualToAnchor:self.leadingAnchor],
      [_view.trailingAnchor constraintEqualToAnchor:self.trailingAnchor],
      [_view.topAnchor constraintEqualToAnchor:self.topAnchor],
      [_view.bottomAnchor constraintEqualToAnchor:self.bottomAnchor]
    ]];
  }
  return self;
}

- (void)setAttributes:(BraveFaviconAttributes*)attributes {
  _attributes = attributes;
  [_view configureWithAttributes:attributes.attributes];
}

- (void)setMonogramFont:(UIFont*)font {
  _monogramFont = font;
  [_view setFont:font];
}

- (CGSize)intrinsicContentSize {
  return [_view intrinsicContentSize];
}
@end
