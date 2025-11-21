/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_browser.h"
#include "brave/browser/ui/brave_tab_strip_model_delegate.h"
#include "brave/browser/ui/tabs/brave_tab_strip_model.h"
#include "brave/browser/ui/tabs/features.h"
#include "brave/browser/ui/tabs/shared_pinned_tab_service.h"
#include "brave/browser/ui/tabs/shared_pinned_tab_service_factory.h"
#include "chrome/browser/ui/browser_command_controller.h"
#include "chrome/browser/ui/tabs/features.h"

#define BrowserTabStripModelDelegate BraveTabStripModelDelegate
#define DeprecatedCreateOwnedForTesting DeprecatedCreateOwnedForTesting_Unused
#define BRAVE_BROWSER_ON_WINDOW_CLOSING                                       \
  if (base::FeatureList::IsEnabled(tabs::features::kBraveSharedPinnedTabs)) { \
    auto* shared_pinned_tab_service =                                         \
        SharedPinnedTabServiceFactory::GetForProfile(profile());              \
    if (shared_pinned_tab_service) {                                          \
      shared_pinned_tab_service->BrowserClosing(tab_strip_model());           \
    }                                                                         \
  }

#include <chrome/browser/ui/browser.cc>

#undef DeprecatedCreateOwnedForTesting
#undef BrowserTabStripModelDelegate
#undef BRAVE_BROWSER_ON_WINDOW_CLOSING

// static
std::unique_ptr<Browser> Browser::DeprecatedCreateOwnedForTesting(
    const CreateParams& params) {
  CHECK_IS_TEST();
  // If this is failing, a caller is trying to create a browser when creation is
  // not possible, e.g. using the wrong profile or during shutdown. The caller
  // should handle this; see e.g. crbug.com/1141608 and crbug.com/1261628.
  CHECK_EQ(CreationStatus::kOk, GetCreationStatusForProfile(params.profile));
  return std::make_unique<BraveBrowser>(params);
}
