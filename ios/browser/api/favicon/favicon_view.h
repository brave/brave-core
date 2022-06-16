/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_FAVICON_FAVICON_VIEW_H_
#define BRAVE_IOS_BROWSER_API_FAVICON_FAVICON_VIEW_H_

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

@class BraveFaviconAttributes;

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
NS_SWIFT_NAME(FavIconView)
@interface BraveFaviconView : UIView
/// Configures this view with given attributes.
@property(nonatomic, nullable) BraveFaviconAttributes* attributes;
/// Sets monogram font.
@property(nonatomic) UIFont* monogramFont;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_FAVICON_FAVICON_VIEW_H_
