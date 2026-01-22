/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ephemeral_storage/ephemeral_storage_tab_helper.h"

#include "base/check.h"
#include "base/feature_list.h"
#include "brave/browser/ephemeral_storage/ephemeral_storage_service_factory.h"
#include "brave/components/brave_shields/core/browser/brave_shields_utils.h"
#include "brave/components/ephemeral_storage/ephemeral_storage_service.h"
#include "chrome/browser/content_settings/cookie_settings_factory.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "net/base/features.h"
#include "net/base/url_util.h"

#if BUILDFLAG(IS_ANDROID)
#include "chrome/browser/android/tab_android.h"
#include "chrome/browser/ui/android/tab_model/tab_model.h"
#include "chrome/browser/ui/android/tab_model/tab_model_list.h"
#endif

using content::BrowserContext;
using content::NavigationHandle;
using content::WebContents;

namespace ephemeral_storage {


#if BUILDFLAG(IS_ANDROID)
namespace {

// Returns the TabModel that was used, or nullptr if none found.
TabModel* AddTabModelObserver(WebContents* web_contents,
                              EphemeralStorageTabHelper* tab_helper) {
  if (!web_contents) {
    return nullptr;
  }

  auto* current_profile =
      Profile::FromBrowserContext(web_contents->GetBrowserContext());
  if (!current_profile) {
    return nullptr;
  }

  for (TabModel* tab_model : TabModelList::models()) {
    if (tab_model->GetProfile() != current_profile) {
      continue;
    }
    const size_t tab_count = tab_model->GetTabCount();
    for (size_t index = 0; index < tab_count; index++) {
      auto* tab = tab_model->GetTabAt(index);
      if (!tab) {
        continue;
      }
      tab_model->AddObserver(tab_helper);
      return tab_model;
    }
  }

  return nullptr;
}

void RemoveTabModelObserver(TabModel* tab_model,
                            EphemeralStorageTabHelper* tab_helper) {
  if (!tab_model) {
    return;
  }
  // Verify TabModel is still valid (not destroyed) before using it.
  // This protects against dangling pointers during browser shutdown.
  for (TabModel* model : TabModelList::models()) {
    if (model == tab_model) {
      tab_model->RemoveObserver(tab_helper);
      return;
    }
  }
}

}  // namespace
#endif

// EphemeralStorageTabHelper helps to manage the lifetime of ephemeral storage.
// For more information about the design of ephemeral storage please see the
// design document at:
// https://github.com/brave/brave-browser/wiki/Ephemeral-Storage-Design
EphemeralStorageTabHelper::EphemeralStorageTabHelper(WebContents* web_contents)
    : WebContentsObserver(web_contents),
      content::WebContentsUserData<EphemeralStorageTabHelper>(*web_contents),
      host_content_settings_map_(HostContentSettingsMapFactory::GetForProfile(
          web_contents->GetBrowserContext())),
      cookie_settings_(CookieSettingsFactory::GetForProfile(
          Profile::FromBrowserContext(web_contents->GetBrowserContext()))) {
  DCHECK(base::FeatureList::IsEnabled(net::features::kBraveEphemeralStorage));
#if BUILDFLAG(IS_ANDROID)
  registered_tab_model_ = AddTabModelObserver(web_contents, this);
#endif
  // The URL might not be empty if this is a restored WebContents, for instance.
  // In that case we want to make sure it has valid ephemeral storage.
  const GURL& url = web_contents->GetLastCommittedURL();
  const std::string ephemeral_storage_domain =
      net::URLToEphemeralStorageDomain(url);
  CreateEphemeralStorageAreasForDomainAndURL(ephemeral_storage_domain, url);
  UpdateShieldsState(url);
}

EphemeralStorageTabHelper::~EphemeralStorageTabHelper() {
#if BUILDFLAG(IS_ANDROID)
  // Always remove observer in destructor using the stored TabModel pointer.
  // We can't rely on web_contents() here as it may already be destroyed.
  RemoveTabModelObserver(registered_tab_model_, this);
#endif
}

std::optional<base::UnguessableToken>
EphemeralStorageTabHelper::GetEphemeralStorageToken(const url::Origin& origin) {
  if (auto* ephemeral_storage_service =
          EphemeralStorageServiceFactory::GetForContext(
              web_contents()->GetBrowserContext())) {
    return ephemeral_storage_service->Get1PESToken(origin);
  }
  return std::nullopt;
}

void EphemeralStorageTabHelper::EnforceFirstPartyStorageCleanup(StorageCleanupSource source) {
  if (tld_ephemeral_lifetime_) {
    tld_ephemeral_lifetime_->EnforceFirstPartyStorageCleanup(source);
  }
}

void EphemeralStorageTabHelper::DidStartNavigation(
    NavigationHandle* navigation_handle) {
  if (!navigation_handle->IsInMainFrame() ||
      navigation_handle->IsSameDocument()) {
    return;
  }

  CreateProvisionalTLDEphemeralLifetime(navigation_handle);
}

void EphemeralStorageTabHelper::DidRedirectNavigation(
    NavigationHandle* navigation_handle) {
  if (!navigation_handle->IsInMainFrame() ||
      navigation_handle->IsSameDocument()) {
    return;
  }

  CreateProvisionalTLDEphemeralLifetime(navigation_handle);
}

void EphemeralStorageTabHelper::DidFinishNavigation(
    NavigationHandle* navigation_handle) {
  if (!navigation_handle->IsInMainFrame() ||
      navigation_handle->IsSameDocument()) {
    return;
  }

  // Clear all provisional ephemeral lifetimes. A committed ephemeral lifetime
  // is created in ReadyToCommitNavigation().
  provisional_tld_ephemeral_lifetimes_.clear();
}

void EphemeralStorageTabHelper::ReadyToCommitNavigation(
    NavigationHandle* navigation_handle) {
  if (!navigation_handle->IsInMainFrame()) {
    return;
  }
  if (navigation_handle->IsSameDocument()) {
    return;
  }

  const GURL& new_url = navigation_handle->GetURL();
  const GURL& last_committed_url = web_contents()->GetLastCommittedURL();

  std::string new_domain = net::URLToEphemeralStorageDomain(new_url);
  std::string previous_domain =
      net::URLToEphemeralStorageDomain(last_committed_url);
  if (new_domain != previous_domain) {
    // Create new storage areas for new ephemeral storage domain.
    CreateEphemeralStorageAreasForDomainAndURL(new_domain, new_url);
  }
  UpdateShieldsState(new_url);
}

void EphemeralStorageTabHelper::CreateEphemeralStorageAreasForDomainAndURL(
    const std::string& new_domain,
    const GURL& new_url) {
  if (new_url.is_empty()) {
    return;
  }

  auto* browser_context = web_contents()->GetBrowserContext();
  auto* site_instance = web_contents()->GetSiteInstance();

  tld_ephemeral_lifetime_ = TLDEphemeralLifetime::GetOrCreate(
      {browser_context, new_domain,
       site_instance->GetStoragePartitionConfig()});
}

void EphemeralStorageTabHelper::CreateProvisionalTLDEphemeralLifetime(
    NavigationHandle* navigation_handle) {
  if (!base::FeatureList::IsEnabled(
          net::features::kBraveProvisionalTLDEphemeralLifetime)) {
    return;
  }

  const GURL& url = navigation_handle->GetURL();
  if (!url.SchemeIsHTTPOrHTTPS()) {
    return;
  }

  const std::string new_domain = net::URLToEphemeralStorageDomain(url);
  if (new_domain.empty()) {
    return;
  }

  auto* browser_context = web_contents()->GetBrowserContext();
  auto* site_instance = web_contents()->GetSiteInstance();

  provisional_tld_ephemeral_lifetimes_.emplace(
      TLDEphemeralLifetime::GetOrCreate(
          {browser_context, new_domain,
           site_instance->GetStoragePartitionConfig()}));
}

void EphemeralStorageTabHelper::UpdateShieldsState(const GURL& url) {
  if (!host_content_settings_map_ || !tld_ephemeral_lifetime_) {
    return;
  }
  const bool shields_enabled =
      brave_shields::GetBraveShieldsEnabled(host_content_settings_map_, url);
  const bool cookies_restricted =
      brave_shields::GetCookieControlType(host_content_settings_map_,
                                          cookie_settings_.get(), url) !=
      brave_shields::ControlType::ALLOW;
  tld_ephemeral_lifetime_->SetShieldsStateOnHost(
      url.host(), shields_enabled && cookies_restricted);
}

#if BUILDFLAG(IS_ANDROID)
void EphemeralStorageTabHelper::WillCloseTab(TabAndroid* tab) {
  if (!tab || tab->web_contents() != web_contents()) {
    return;
  }
  // Reset TLDEphemeralLifetime when a tab closes, since on Android
  // it may be invoked much later after the tab has actually closed.
  provisional_tld_ephemeral_lifetimes_.clear();
  tld_ephemeral_lifetime_.reset();
  weak_factory_.InvalidateWeakPtrs();
  RemoveTabModelObserver(registered_tab_model_, this);
  registered_tab_model_ = nullptr;
}
#endif

WEB_CONTENTS_USER_DATA_KEY_IMPL(EphemeralStorageTabHelper);

}  // namespace ephemeral_storage
