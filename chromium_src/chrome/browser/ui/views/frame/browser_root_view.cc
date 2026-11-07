/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/frame/browser_root_view.h"

#include "chrome/browser/defaults.h"
#include "chrome/browser/profiles/profile.h"

class PrefService;

// Defined in //brave/browser/ui:brave_tab_prefs, linked through
// //brave/browser:core.
namespace brave_tabs {
bool IsScrollableHorizontalTabStripEnabled(const PrefService* prefs);
}  // namespace brave_tabs

// Disable scroll-event-changes-tab when the scrollable horizontal tab strip is
// active (feature and user pref).
#define kScrollEventChangesTab                             \
  kScrollEventChangesTab &&                                \
      (!brave_tabs::IsScrollableHorizontalTabStripEnabled( \
           browser_view_->GetProfile()->GetPrefs()) ||     \
       event.IsControlDown())

#include <chrome/browser/ui/views/frame/browser_root_view.cc>

#undef kScrollEventChangesTab
