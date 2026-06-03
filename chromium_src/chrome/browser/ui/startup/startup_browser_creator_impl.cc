/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/startup/startup_browser_creator_impl.h"

#include "brave/browser/ui/startup/brave_startup_tab_provider_impl.h"
#include "brave/components/containers/buildflags/buildflags.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/navigator/browser_navigator_params.h"
#include "chrome/browser/ui/startup/startup_browser_creator.h"
#include "chrome/browser/ui/startup/startup_tab_provider.h"

#if BUILDFLAG(ENABLE_CONTAINERS)
#include "brave/browser/containers/container_specifier_utils.h"
#endif  // BUILDFLAG(ENABLE_CONTAINERS)

namespace {

void BraveModifyStartupTabNavigationParams(const StartupTab& tab,
                                           BrowserWindowInterface* browser,
                                           NavigateParams& params) {
#if BUILDFLAG(ENABLE_CONTAINERS)
  if (!params.storage_partition_config) {
    params.storage_partition_config =
        containers::GetStoragePartitionConfigForContainerSpecifier(
            browser->GetProfile(), tab.container);
  }
#endif
}

}  // namespace

#define StartupTabProviderImpl BraveStartupTabProviderImpl

#include <chrome/browser/ui/startup/startup_browser_creator_impl.cc>

#undef StartupTabProviderImpl
