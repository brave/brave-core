// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/brave_search/brave_search_make_default_tab_helper.h"

#include "brave/ios/browser/brave_search/brave_search_make_default_tab_helper_bridge.h"

BraveSearchMakeDefaultTabHelper::BraveSearchMakeDefaultTabHelper(
    web::WebState* web_state) {}

BraveSearchMakeDefaultTabHelper::~BraveSearchMakeDefaultTabHelper() = default;

void BraveSearchMakeDefaultTabHelper::SetBridge(
    id<BraveSearchMakeDefaultTabHelperBridge> bridge) {
  bridge_ = bridge;
}

bool BraveSearchMakeDefaultTabHelper::GetCanSetDefaultSearchProvider() {
  if (!bridge_) {
    return false;
  }
  return static_cast<bool>([bridge_ getCanSetDefaultSearchProvider]);
}

void BraveSearchMakeDefaultTabHelper::SetIsDefaultSearchProvider() {
  if (bridge_) {
    [bridge_ setIsDefaultSearchProvider];
  }
}
