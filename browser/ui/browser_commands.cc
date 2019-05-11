/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/browser_commands.h"

#include <utility>

#include "brave/browser/tor/tor_profile_service.h"
#include "brave/browser/tor/tor_profile_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/profiles/profile_window.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "components/web_cache/browser/web_cache_manager.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "services/network/public/cpp/features.h"

using content::BrowserContext;
using content::BrowserThread;
using content::NavigationController;
using content::StoragePartition;
using content::WebContents;

namespace {
// Reload the page once we have a new circuit and cleared browsing data.
void NewTorIdentityCallbackReload(WebContents* current_tab) {
  NavigationController& controller = current_tab->GetController();
  controller.Reload(content::ReloadType::BYPASSING_CACHE, true);
}

// Flush everything else in the storage partition.
void NewTorIdentityCallbackClearData(WebContents* current_tab) {
  auto* context = current_tab->GetBrowserContext();
  auto* site = current_tab->GetSiteInstance();
  auto* partition = BrowserContext::GetStoragePartition(context, site);
  auto cookie_delete_filter = network::mojom::CookieDeletionFilter::New();
  // Delete all cookies, irrespective of persistence status.
  cookie_delete_filter->session_control =
    network::mojom::CookieDeletionSessionControl::IGNORE_CONTROL;
  partition->ClearData(
      StoragePartition::REMOVE_DATA_MASK_ALL,
      StoragePartition::QUOTA_MANAGED_STORAGE_MASK_ALL,
      StoragePartition::OriginMatcherFunction(),
      std::move(cookie_delete_filter),
      /*perform_cleanup*/ true,
      /*begin*/ base::Time(),           // beginning of time
      /*end*/ base::Time::Max(),        // end of time
      base::Bind(&NewTorIdentityCallbackReload, current_tab));
}

// Flush any code caches in the storage partition.
void NewTorIdentityCallbackClearCodeCaches(WebContents* current_tab) {
  auto* context = current_tab->GetBrowserContext();
  auto* site = current_tab->GetSiteInstance();
  auto* partition = BrowserContext::GetStoragePartition(context, site);
  partition->ClearCodeCaches(
      /*begin*/ base::Time(),           // beginning of time
      /*end*/ base::Time::Max(),        // end of time
      base::RepeatingCallback<bool(const GURL&)>(),
      base::BindOnce(&NewTorIdentityCallbackClearData, current_tab));
}

// Flush the renderer cache and the HTTP and media caches.
void NewTorIdentityCallback(WebContents* current_tab) {
  int render_process_id = current_tab->GetMainFrame()->GetProcess()->GetID();
  auto* cache_manager = web_cache::WebCacheManager::GetInstance();
  cache_manager->ClearCacheForProcess(render_process_id);

  auto* context = current_tab->GetBrowserContext();
  auto* site = current_tab->GetSiteInstance();
  auto* partition = BrowserContext::GetStoragePartition(context, site);
  base::OnceClosure callback = base::BindOnce(
      &NewTorIdentityCallbackClearCodeCaches, current_tab);
  if (base::FeatureList::IsEnabled(network::features::kNetworkService)) {
    partition->GetNetworkContext()->ClearHttpCache(
        /*begin*/ base::Time(),         // beginning of time
        /*end*/ base::Time::Max(),      // end of time
        /*ClearDataFilter*/ nullptr,
        std::move(callback));
  } else {
    partition->ClearHttpAndMediaCaches(
        /*begin*/ base::Time(),         // beginning of time
        /*end*/ base::Time::Max(),      // end of time
        base::RepeatingCallback<bool(const GURL&)>(),
        std::move(callback));
  }
}
}  // namespace

namespace brave {

void NewOffTheRecordWindowTor(Browser* browser) {
  profiles::SwitchToTorProfile(ProfileManager::CreateCallback());
}

void NewTorIdentity(Browser* browser) {
  Profile* profile = browser->profile();
  DCHECK(profile);
  tor::TorProfileService* service =
    TorProfileServiceFactory::GetForProfile(profile);
  DCHECK(service);
  WebContents* current_tab =
    browser->tab_strip_model()->GetActiveWebContents();
  if (!current_tab)
    return;
  const GURL current_url = current_tab->GetURL();
  service->SetNewTorCircuit(current_url, base::Bind(&NewTorIdentityCallback,
                                                    current_tab));
}

}  // namespace brave
