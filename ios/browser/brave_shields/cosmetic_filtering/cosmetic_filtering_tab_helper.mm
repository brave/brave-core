// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/brave_shields/cosmetic_filtering/cosmetic_filtering_tab_helper.h"

#include <utility>

#include "base/functional/callback.h"
#include "base/strings/sys_string_conversions.h"
#include "brave/ios/browser/brave_shields/cosmetic_filtering/cosmetic_filtering_tab_helper_bridge.h"
#include "ios/web/public/web_state.h"
#include "net/base/apple/url_conversions.h"
#include "url/gurl.h"

CosmeticFilteringTabHelper::CosmeticFilteringTabHelper(
    web::WebState* web_state) {}

CosmeticFilteringTabHelper::~CosmeticFilteringTabHelper() = default;

void CosmeticFilteringTabHelper::SetBridge(
    id<CosmeticFilteringTabHelperBridge> bridge) {
  bridge_ = bridge;
}
