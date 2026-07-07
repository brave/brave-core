/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/unload_controller.h"

#include "brave/browser/ui/tabs/shared_pinned_tab_service.h"
#include "brave/browser/ui/tabs/shared_pinned_tab_service_factory.h"
#include "brave/components/constants/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/tabs/features.h"
#include "components/prefs/pref_service.h"

// SharedPinnedTabService must get a chance to detach/cache shared pinned tabs
// before CloseAllTabs() runs, otherwise they close like ordinary tabs.
#define BRAVE_UNLOAD_CONTROLLER_ON_WINDOW_CLOSING                             \
  if (base::FeatureList::IsEnabled(tabs::kBraveSharedPinnedTabs)) {           \
    auto* shared_pinned_tab_service =                                         \
        SharedPinnedTabServiceFactory::GetForProfile(browser_->GetProfile()); \
    if (shared_pinned_tab_service) {                                          \
      /* When there are only pinned tabs, OnWindowClosing() will be */        \
      /* called again, after we detach all pinned tabs from the */            \
      /* browser from shared_pinned_tab_service->BrowserClosing() */          \
      if (shared_pinned_tab_service->BrowserClosing(                          \
              browser_->GetTabStripModel())) {                                \
        return;                                                               \
      }                                                                       \
    }                                                                         \
  }

#include <chrome/browser/ui/unload_controller.cc>

#undef BRAVE_UNLOAD_CONTROLLER_ON_WINDOW_CLOSING
