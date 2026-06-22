/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/frame/browser_root_view.h"

#include "base/feature_list.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "chrome/browser/defaults.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/tabs/features.h"
#include "components/prefs/pref_service.h"

namespace {

bool IsScrollableHorizontalTabStripEnabled(Profile* profile) {
  return base::FeatureList::IsEnabled(tabs::kBraveScrollableTabStrip) &&
         profile->GetPrefs()->GetBoolean(
             brave_tabs::kScrollableHorizontalTabStrip);
}

}  // namespace

// Disable scroll-event-changes-tab when the scrollable horizontal tab strip is
// active (feature and user pref).
#define kScrollEventChangesTab                                                \
  kScrollEventChangesTab &&                                                   \
      (!IsScrollableHorizontalTabStripEnabled(browser_view_->GetProfile()) || \
       event.IsControlDown())

#include <chrome/browser/ui/views/frame/browser_root_view.cc>

#undef kScrollEventChangesTab
