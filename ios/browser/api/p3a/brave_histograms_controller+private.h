/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_P3A_BRAVE_HISTOGRAMS_CONTROLLER_PRIVATE_H_
#define BRAVE_IOS_BROWSER_API_P3A_BRAVE_HISTOGRAMS_CONTROLLER_PRIVATE_H_

#include "brave/ios/browser/api/p3a/brave_histograms_controller.h"

class ChromeBrowserState;

NS_ASSUME_NONNULL_BEGIN

@interface BraveHistogramsController (Private)
- (instancetype)initWithBrowserState:(ChromeBrowserState*)mainBrowserState;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_P3A_BRAVE_HISTOGRAMS_CONTROLLER_PRIVATE_H_
