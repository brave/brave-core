/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/p3a/brave_p3a_utils.h"

#include "base/metrics/histogram_functions.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/sys_string_conversions.h"
#include "brave/ios/browser/api/p3a/brave_histograms_controller+private.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation BraveP3AUtils {
  ChromeBrowserState* _browserState;
}

- (instancetype)initWithBrowserState:(ChromeBrowserState*)mainBrowserState {
  if ((self = [super init])) {
    _browserState = mainBrowserState;
  }
  return self;
}

- (BraveHistogramsController*)histogramsController {
  return [[BraveHistogramsController alloc] initWithBrowserState:_browserState];
}

@end
