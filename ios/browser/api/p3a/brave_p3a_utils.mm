/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/p3a/brave_p3a_utils.h"

#include "base/metrics/histogram_functions.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/sys_string_conversions.h"
#include "brave/components/p3a/pref_names.h"
#include "brave/ios/browser/api/p3a/brave_histograms_controller+private.h"
#include "components/prefs/pref_service.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation BraveP3AUtils {
  ChromeBrowserState* _browserState;
  PrefService* _localState;
}

- (instancetype)initWithBrowserState:(ChromeBrowserState*)mainBrowserState
                          localState:(PrefService*)localState {
  if ((self = [super init])) {
    _browserState = mainBrowserState;
    _localState = localState;
  }
  return self;
}

- (bool)isP3AEnabled {
  return _localState->GetBoolean(brave::kP3AEnabled);
}

- (void)setIsP3AEnabled:(bool)isP3AEnabled {
  _localState->SetBoolean(brave::kP3AEnabled, isP3AEnabled);
}

- (bool)isNoticeAcknowledged {
  return _localState->GetBoolean(brave::kP3ANoticeAcknowledged);
}

- (void)setIsNoticeAcknowledged:(bool)isNoticeAcknowledged {
  _localState->SetBoolean(brave::kP3ANoticeAcknowledged, isNoticeAcknowledged);
}

- (BraveHistogramsController*)histogramsController {
  return [[BraveHistogramsController alloc] initWithBrowserState:_browserState];
}

@end
