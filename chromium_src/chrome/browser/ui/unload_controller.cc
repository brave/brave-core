/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/unload_controller.h"

#include "brave/browser/ui/tabs/shared_pinned_tab_service.h"
#include "brave/browser/ui/tabs/shared_pinned_tab_service_factory.h"
#include "brave/components/constants/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/features.h"
#include "components/prefs/pref_service.h"

namespace {

// SharedPinnedTabService must get a chance to detach/cache shared pinned tabs
// before CloseAllTabs() runs, otherwise they close like ordinary tabs.
bool MaybeHandleSharedPinnedTabsClosing(Browser* browser) {
  if (!base::FeatureList::IsEnabled(tabs::kBraveSharedPinnedTabs)) {
    return false;
  }
  auto* shared_pinned_tab_service =
      SharedPinnedTabServiceFactory::GetForProfile(browser->GetProfile());
  if (!shared_pinned_tab_service) {
    return false;
  }
  // When there are only pinned tabs, OnWindowClosing() will be called again,
  // after we detach all pinned tabs from the browser from
  // shared_pinned_tab_service->BrowserClosing()
  return shared_pinned_tab_service->BrowserClosing(browser->GetTabStripModel());
}

}  // namespace

#include <chrome/browser/ui/unload_controller.cc>
