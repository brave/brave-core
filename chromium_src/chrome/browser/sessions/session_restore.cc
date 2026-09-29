/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ephemeral_storage/ephemeral_storage_service_factory.h"
#include "brave/components/containers/buildflags/buildflags.h"
#include "brave/components/ephemeral_storage/ephemeral_storage_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sessions/tab_restore_service_factory.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/navigator/browser_navigator_params.h"
#include "chrome/browser/ui/startup/startup_tab.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "components/sessions/core/session_types.h"
#include "components/sessions/core/tab_restore_service.h"
#include "content/public/browser/web_contents.h"
#include "net/base/url_util.h"
#include "url/gurl.h"

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

bool IsScheduledForCleanup(const GURL& url, Profile* profile) {
  auto* service = EphemeralStorageServiceFactory::GetForContext(profile);
  if (!service) {
    return false;
  }

  return service->IsScheduledForCleanup(
      net::URLToEphemeralStorageDomain(url));
}

// Skipping tabs scheduled for ephemeral storage cleanup can leave a restored
// browser with no tabs at all (e.g. a single-tab window whose only tab was
// scheduled for cleanup). Adds a new tab if that happens so the caller has a
// tab to show/activate. Returns the tab's WebContents, or nullptr if the
// browser already has tabs.
content::WebContents* MaybeAddFallbackTabIfEmpty(
    BrowserWindowInterface* browser) {
  if (!browser->GetTabStripModel()->empty()) {
    return nullptr;
  }
  return chrome::AddAndReturnTabAt(browser, GURL(), -1, true);
}

}  // namespace

#include <chrome/browser/sessions/session_restore.cc>
