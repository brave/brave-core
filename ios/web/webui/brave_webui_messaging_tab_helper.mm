// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/web/webui/brave_webui_messaging_tab_helper.h"

BraveWebUIMessagingTabHelper::BraveWebUIMessagingTabHelper(
    web::WebState* web_state) {}

BraveWebUIMessagingTabHelper::~BraveWebUIMessagingTabHelper() = default;

id<BraveWebUIMessagingTabHelperDelegate>
BraveWebUIMessagingTabHelper::GetBridgingDelegate() {
  return bridge_;
}

void BraveWebUIMessagingTabHelper::SetBridgingDelegate(
    id<BraveWebUIMessagingTabHelperDelegate> delegate) {
  bridge_ = delegate;
}
