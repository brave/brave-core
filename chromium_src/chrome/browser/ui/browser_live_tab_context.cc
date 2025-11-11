// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/browser_live_tab_context.h"

#include "base/feature_list.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/tabs/features.h"
#include "brave/browser/ui/tabs/public/constants.h"
#include "chrome/browser/ui/tab_ui_helper.h"
#include "chrome/browser/ui/tabs/public/tab_features.h"

#define GetExtraDataForTab GetExtraDataForTab_ChromiumImpl

#include <chrome/browser/ui/browser_live_tab_context.cc>

#undef GetExtraDataForTab

std::map<std::string, std::string> BrowserLiveTabContext::GetExtraDataForTab(
    int index) const {
  auto extra_data = GetExtraDataForTab_ChromiumImpl(index);
  if (!base::FeatureList::IsEnabled(tabs::kBraveRenamingTabs)) {
    return extra_data;
  }

  // Add custom title data to extra data for tabs if it exists.
  auto* tab_interface =
      browser_->GetFeatures().tab_strip_model()->GetTabAtIndex(index);
  CHECK(tab_interface);
  auto* tab_ui_helper = tab_interface->GetTabFeatures()->tab_ui_helper();
  CHECK(tab_ui_helper);
  if (tab_ui_helper->has_custom_title()) {
    extra_data[tabs::kBraveTabCustomTitleExtraDataKey] =
        base::UTF16ToUTF8(tab_ui_helper->GetTitle());
  }

  return extra_data;
}
