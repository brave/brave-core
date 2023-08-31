// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/browser_state/brave_browser_state_manager_impl.h"

#include "brave/ios/browser/url_sanitizer/url_sanitizer_service_factory.h"
#include "ios/chrome/browser/shared/model/browser_state/chrome_browser_state.h"

void BraveBrowserStateManagerImpl::DoFinalInitForServices(
    ChromeBrowserState* browser_state) {
  ChromeBrowserStateManagerImpl::DoFinalInitForServices(browser_state);
  brave::URLSanitizerServiceFactory::GetServiceForState(browser_state);
}
