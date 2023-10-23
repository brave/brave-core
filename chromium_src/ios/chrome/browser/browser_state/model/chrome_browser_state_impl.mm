/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "ios/chrome/browser/browser_state/model/chrome_browser_state_impl.h"

#include "ios/chrome/browser/browser_state/model/off_the_record_chrome_browser_state_impl.h"
#include "ios/chrome/browser/policy/browser_policy_connector_ios.h"
#include "ios/chrome/browser/policy/browser_state_policy_connector.h"
#include "ios/chrome/browser/policy/browser_state_policy_connector_factory.h"
#include "ios/chrome/browser/shared/model/application_context/application_context.h"

#define BrowserPolicyConnectorIOS \
  if (false) {                    \
  BrowserPolicyConnectorIOS

#undef BuildBrowserStatePolicyConnector
#define BuildBrowserStatePolicyConnector(ARG1, ARG2, ARG3) \
  nullptr;                                                 \
  }

#define GetPolicyConnector GetPolicyConnector_ChromiumImpl

#include "src/ios/chrome/browser/browser_state/model/chrome_browser_state_impl.mm"

#undef GetPolicyConnector
#undef BuildBrowserStatePolicyConnector
#undef BrowserPolicyConnectorIOS

BrowserStatePolicyConnector* ChromeBrowserStateImpl::GetPolicyConnector() {
  return nullptr;
}
