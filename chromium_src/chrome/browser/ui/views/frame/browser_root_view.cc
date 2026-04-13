/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/frame/browser_root_view.h"

#include "base/feature_list.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "chrome/browser/defaults.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/features.h"
#include "components/prefs/pref_service.h"

namespace {

bool IsScrollableHorizontalTabStripEnabled(Browser* browser) {
  return base::FeatureList::IsEnabled(tabs::kBraveScrollableTabStrip) &&
         browser->GetProfile()->GetPrefs()->GetBoolean(
             brave_tabs::kScrollableHorizontalTabStrip);
}

}  // namespace

// Workaround for vertical tabs to work with drag&drop of text/links.
#define ConvertPointToTarget(THIS, TARGET_GETTER, POINT)                    \
  if (views::View* target_v = TARGET_GETTER;                                \
      tabs::utils::ShouldShowBraveVerticalTabs(browser_view_->browser()) && \
      (target_v == browser_view_->tab_strip_view() ||                       \
       !THIS->Contains(target_v))) {                                        \
    ConvertPointToScreen(THIS, POINT);                                      \
    ConvertPointFromScreen(target_v, POINT);                                \
  } else {                                                                  \
    ConvertPointToTarget(THIS, target_v, POINT);                            \
  }

// Disable scroll-event-changes-tab when the scrollable horizontal tab strip is
// active (feature and user pref).
#define kScrollEventChangesTab                                             \
  kScrollEventChangesTab &&                                                \
      (!IsScrollableHorizontalTabStripEnabled(browser_view_->browser()) || \
       event.IsControlDown())

#include <chrome/browser/ui/views/frame/browser_root_view.cc>

#undef kScrollEventChangesTab
#undef ConvertPointToTarget
