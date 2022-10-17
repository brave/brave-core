/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_P3A_BRAVE_HISTOGRAMS_CONTROLLER_H_
#define BRAVE_IOS_BROWSER_API_P3A_BRAVE_HISTOGRAMS_CONTROLLER_H_

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

/// Displays the `chrome://histograms` web page to verify P3A histograms
OBJC_EXPORT
@interface BraveHistogramsController : UIViewController
- (instancetype)init NS_UNAVAILABLE;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_P3A_BRAVE_HISTOGRAMS_CONTROLLER_H_
