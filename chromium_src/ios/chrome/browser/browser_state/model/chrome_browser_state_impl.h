/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_IOS_CHROME_BROWSER_BROWSER_STATE_MODEL_CHROME_BROWSER_STATE_IMPL_H_
#define BRAVE_CHROMIUM_SRC_IOS_CHROME_BROWSER_BROWSER_STATE_MODEL_CHROME_BROWSER_STATE_IMPL_H_

#include "ios/chrome/browser/shared/model/browser_state/chrome_browser_state.h"

#define GetPolicyConnector           \
  GetPolicyConnector_ChromiumImpl(); \
  BrowserStatePolicyConnector* GetPolicyConnector

#include "src/ios/chrome/browser/browser_state/model/chrome_browser_state_impl.h"  // IWYU pragma: export
#undef GetPolicyConnector

#endif  // BRAVE_CHROMIUM_SRC_IOS_CHROME_BROWSER_BROWSER_STATE_MODEL_CHROME_BROWSER_STATE_IMPL_H_
