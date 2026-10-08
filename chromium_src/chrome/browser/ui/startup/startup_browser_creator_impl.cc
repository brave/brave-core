/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/startup/startup_browser_creator_impl.h"

#include "brave/browser/ui/startup/brave_startup_tab_provider_impl.h"
#include "brave/components/containers/buildflags/buildflags.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/navigator/browser_navigator.h"
#include "chrome/browser/ui/navigator/browser_navigator_params.h"
#include "chrome/browser/ui/startup/startup_browser_creator.h"
#include "chrome/browser/ui/startup/startup_tab_provider.h"
#include "url/gurl.h"

#if BUILDFLAG(ENABLE_CONTAINERS)
#include "brave/browser/containers/container_specifier_utils.h"
#endif  // BUILDFLAG(ENABLE_CONTAINERS)

// Returns true if the ephemeral storage for |url|'s domain is scheduled for
// cleanup, in which case the tab should not be restored.
// Implemented in
// browser/ephemeral_storage/ephemeral_storage_session_restore_helpers.cc so
// that the upstream sessions target does not depend on Brave ephemeral storage.
bool BraveIsScheduledForCleanup(const GURL& url, Profile* profile);

namespace {

void BraveNavigateStartupTab(const StartupTab& tab,
                             BrowserWindowInterface* browser,
                             NavigateParams& params) {
#if BUILDFLAG(ENABLE_CONTAINERS)
  if (!params.storage_partition_config) {
    params.storage_partition_config =
        containers::GetStoragePartitionConfigForContainerSpecifier(
            browser->GetProfile(), tab.container);
  }
#endif
  Navigate(&params);
}

}  // namespace

#define StartupTabProviderImpl BraveStartupTabProviderImpl

#include <chrome/browser/ui/startup/startup_browser_creator_impl.cc>

#undef StartupTabProviderImpl
