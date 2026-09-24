/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/focus_mode/focus_mode_utils.h"

#include "base/feature_list.h"
#include "brave/browser/ui/focus_mode/focus_mode_controller.h"
#include "brave/browser/ui/focus_mode/focus_mode_features.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"

bool BrowserSupportsFocusMode(const BrowserWindowInterface* browser) {
  return base::FeatureList::IsEnabled(features::kBraveFocusMode) && browser &&
         browser->GetType() == BrowserWindowInterface::TYPE_NORMAL;
}

bool IsFocusModeEnabled(const BrowserWindowInterface* browser) {
  if (!browser) {
    return false;
  }
  auto* controller = browser->GetFeatures().focus_mode_controller();
  return controller && controller->IsEnabled();
}
