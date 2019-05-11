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
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"

using content::BrowserThread;
using content::NavigationController;
using content::WebContents;

namespace {
void NewTorIdentityCallbackDone(WebContents* current_tab) {
  NavigationController& controller = current_tab->GetController();
  controller.Reload(content::ReloadType::BYPASSING_CACHE, true);
}

void NewTorIdentityCallback(WebContents* current_tab) {
  auto* context = current_tab->GetBrowserContext();
  auto* site = current_tab->GetSiteInstance();
  auto* partition =
    content::BrowserContext::GetStoragePartition(context, site);
  auto cookie_delete_filter = network::mojom::CookieDeletionFilter::New();
  // Delete all cookies, irrespective of persistence status.
  cookie_delete_filter->session_control =
    network::mojom::CookieDeletionSessionControl::IGNORE_CONTROL;
  partition->ClearData(
      content::StoragePartition::REMOVE_DATA_MASK_ALL,
      content::StoragePartition::QUOTA_MANAGED_STORAGE_MASK_ALL,
      content::StoragePartition::OriginMatcherFunction(),
      std::move(cookie_delete_filter),
      /*perform_cleanup*/ true,
      /*begin*/ base::Time(),           // beginning of time
      /*end*/ base::Time::Max(),        // end of time
      base::Bind(&NewTorIdentityCallbackDone, current_tab));
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
