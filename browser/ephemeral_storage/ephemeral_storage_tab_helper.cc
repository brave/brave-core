/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ephemeral_storage/ephemeral_storage_tab_helper.h"

#include "base/feature_list.h"
#include "base/unguessable_token.h"
#include "brave/browser/ephemeral_storage/ephemeral_storage_service_factory.h"
#include "brave/components/brave_shields/content/browser/brave_shields_util.h"
#include "brave/components/ephemeral_storage/ephemeral_storage_service.h"
#include "chrome/browser/content_settings/cookie_settings_factory.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "net/base/features.h"
#include "net/base/url_util.h"

using content::BrowserContext;
using content::NavigationHandle;
using content::WebContents;

namespace ephemeral_storage {

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

  // The URL might not be empty if this is a restored WebContents, for instance.
  // In that case we want to make sure it has valid ephemeral storage.
  const GURL& url = web_contents->GetLastCommittedURL();
  const std::string ephemeral_storage_domain =
      net::URLToEphemeralStorageDomain(url);
  CreateEphemeralStorageAreasForDomainAndURL(ephemeral_storage_domain, url);
  UpdateShieldsState(url);
}

EphemeralStorageTabHelper::~EphemeralStorageTabHelper() = default;

std::optional<base::UnguessableToken>
EphemeralStorageTabHelper::GetEphemeralStorageToken(
    std::optional<url::Origin> origin) {
  if (auto tab_es_token = TakeEphemeralStorageTokenForNewTab()) {
    return tab_es_token;
  }

  if (ephemeral_storage_token_) {
    return ephemeral_storage_token_;
  }

  if (auto* ephemeral_storage_service =
          EphemeralStorageServiceFactory::GetForContext(
              web_contents()->GetBrowserContext());
      ephemeral_storage_service && origin) {
    return ephemeral_storage_service->Get1PESToken(*origin);
  }
  return std::nullopt;
}

void EphemeralStorageTabHelper::SetEphemeralStorageToken(
    std::optional<base::UnguessableToken> token) {
  CHECK(!ephemeral_storage_token_);
  ephemeral_storage_token_ = token;
}

void EphemeralStorageTabHelper::GenerateEphemeralStorageTokenForNewTab() {
  ephemeral_storage_token_for_new_tab_ = base::UnguessableToken::Create();
}

std::optional<base::UnguessableToken>
EphemeralStorageTabHelper::TakeEphemeralStorageTokenForNewTab() {
  LOG(ERROR) << "EphemeralStorageTabHelper::TakeEphemeralStorageTokenForNewTab";
  if (ephemeral_storage_token_for_new_tab_) {
    auto ephemeral_storage_token_for_new_tab =
        std::move(ephemeral_storage_token_for_new_tab_);
    LOG(ERROR) << "EPHEMERAL!";
    return ephemeral_storage_token_for_new_tab;
  }

  if (!ephemeral_storage_token_) {
    if (auto* true_opener_wc =
            GetWebContents().GetFirstWebContentsInLiveOriginalOpenerChain()) {
      if (auto* opener_es_tab_helper =
              EphemeralStorageTabHelper::FromWebContents(true_opener_wc)) {
        ephemeral_storage_token_ =
            opener_es_tab_helper->GetEphemeralStorageToken(std::nullopt);
      } else {
        LOG(ERROR) << "NO OPENER_ES_TAB_HELPER";
      }
    } else {
      LOG(ERROR) << "NO TRUE_OPENER_WC, usual opener: "
                 << GetWebContents().GetOpener();
    }
  }

  return ephemeral_storage_token_;
}

void EphemeralStorageTabHelper::WebContentsDestroyed() {}

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
      browser_context, new_domain, site_instance->GetStoragePartitionConfig());
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

WEB_CONTENTS_USER_DATA_KEY_IMPL(EphemeralStorageTabHelper);

}  // namespace ephemeral_storage
