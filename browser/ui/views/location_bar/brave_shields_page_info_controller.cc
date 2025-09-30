/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/location_bar/brave_shields_page_info_controller.h"

BraveShieldsPageInfoController::BraveShieldsPageInfoController() = default;
BraveShieldsPageInfoController::~BraveShieldsPageInfoController() = default;

void BraveShieldsPageInfoController::UpdateWebContents(
    content::WebContents* web_contents) {
  shields_observation_.Reset();
  if (web_contents) {
    auto* shields_tab_helper =
        brave_shields::BraveShieldsTabHelper::FromWebContents(web_contents);
    if (shields_tab_helper) {
      shields_observation_.Observe(shields_tab_helper);
    }
  }
}

void BraveShieldsPageInfoController::OnResourcesChanged() {}

void BraveShieldsPageInfoController::OnRepeatedReloadsDetected() {
  // TODO: Open the PageInfo bubble with the Shields WebUI visible and with the
  // following URL query: "?mode=afterRepeatedReloads".
}
