/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ephemeral_storage/brave_ephemeral_storage_service_delegate.h"

#include <utility>

#include "brave/components/brave_shields/content/browser/brave_shields_util.h"
#include "chrome/browser/browsing_data/chrome_browsing_data_remover_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browsing_data_filter_builder.h"
#include "content/public/browser/browsing_data_remover.h"
#include "content/public/browser/dom_storage_context.h"
#include "net/base/features.h"
#include "net/base/schemeful_site.h"
#include "url/origin.h"

#if !BUILDFLAG(IS_ANDROID)
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#endif

namespace ephemeral_storage {

BraveEphemeralStorageServiceDelegate::BraveEphemeralStorageServiceDelegate(
    content::BrowserContext* context,
    HostContentSettingsMap* host_content_settings_map,
    scoped_refptr<content_settings::CookieSettings> cookie_settings)
    : context_(context),
      host_content_settings_map_(host_content_settings_map),
      cookie_settings_(std::move(cookie_settings)) {
  DCHECK(context_);
  DCHECK(host_content_settings_map_);
  DCHECK(cookie_settings_);
}

BraveEphemeralStorageServiceDelegate::~BraveEphemeralStorageServiceDelegate() {
#if !BUILDFLAG(IS_ANDROID)
  BrowserList::RemoveObserver(this);
#endif
}

void BraveEphemeralStorageServiceDelegate::CleanupTLDEphemeralArea(
    const TLDEphemeralAreaKey& key) {
  DVLOG(1) << __func__ << " " << key.first << " " << key.second;
  auto* storage_partition = context_->GetStoragePartition(key.second);
  if (!storage_partition) {
    return;
  }
  auto filter = network::mojom::CookieDeletionFilter::New();
  filter->ephemeral_storage_domain = key.first;
  storage_partition->GetCookieManagerForBrowserProcess()->DeleteCookies(
      std::move(filter), base::NullCallback());

  const GURL https_url(base::StrCat({"https://", key.first}));
  if (brave_shields::GetCookieControlType(host_content_settings_map_,
                                          cookie_settings_.get(), https_url) ==
      brave_shields::ControlType::ALLOW) {
    // All cookies are allowed, Ephemeral Storage is effectively disabled.
    return;
  }

  const GURL http_url(base::StrCat({"http://", key.first}));

  // Only cleanup StorageKey-aware areas.
  content::BrowsingDataRemover::DataType data_to_remove =
      content::BrowsingDataRemover::DATA_TYPE_DOM_STORAGE;

  content::BrowsingDataRemover::OriginType origin_type =
      content::BrowsingDataRemover::ORIGIN_TYPE_UNPROTECTED_WEB |
      content::BrowsingDataRemover::ORIGIN_TYPE_PROTECTED_WEB;

  // Cookies are partitioned and cleaned separately.
  data_to_remove &= ~content::BrowsingDataRemover::DATA_TYPE_COOKIES;

  auto filter_builder = content::BrowsingDataFilterBuilder::Create(
      content::BrowsingDataFilterBuilder::Mode::kDelete,
      content::BrowsingDataFilterBuilder::OriginMatchingMode::
          kThirdPartiesOnly);
  filter_builder->AddOrigin(url::Origin::Create(https_url));
  filter_builder->AddOrigin(url::Origin::Create(http_url));

  content::BrowsingDataRemover* remover = context_->GetBrowsingDataRemover();
  remover->RemoveWithFilter(base::Time(), base::Time::Max(), data_to_remove,
                            origin_type, std::move(filter_builder));
}

void BraveEphemeralStorageServiceDelegate::CleanupFirstPartyStorageArea(
    const std::string& registerable_domain) {
  DVLOG(1) << __func__ << " " << registerable_domain;
  DCHECK(base::FeatureList::IsEnabled(
             net::features::kBraveForgetFirstPartyStorage) ||
         base::FeatureList::IsEnabled(
             net::features::kThirdPartyStoragePartitioning));

  content::BrowsingDataRemover::DataType data_to_remove =
      content::BrowsingDataRemover::DATA_TYPE_COOKIES |
      content::BrowsingDataRemover::DATA_TYPE_CACHE |
      content::BrowsingDataRemover::DATA_TYPE_MEDIA_LICENSES |
      content::BrowsingDataRemover::DATA_TYPE_DOM_STORAGE |
      content::BrowsingDataRemover::DATA_TYPE_ATTRIBUTION_REPORTING |
      content::BrowsingDataRemover::DATA_TYPE_PRIVACY_SANDBOX |
      content::BrowsingDataRemover::DATA_TYPE_PRIVACY_SANDBOX_INTERNAL |
      content::BrowsingDataRemover::DATA_TYPE_AVOID_CLOSING_CONNECTIONS |
      chrome_browsing_data_remover::DATA_TYPE_SITE_DATA;

  content::BrowsingDataRemover::OriginType origin_type =
      content::BrowsingDataRemover::ORIGIN_TYPE_UNPROTECTED_WEB |
      content::BrowsingDataRemover::ORIGIN_TYPE_PROTECTED_WEB;

  auto filter_builder = content::BrowsingDataFilterBuilder::Create(
      content::BrowsingDataFilterBuilder::Mode::kDelete);
  filter_builder->AddRegisterableDomain(registerable_domain);

  content::BrowsingDataRemover* remover = context_->GetBrowsingDataRemover();
  remover->RemoveWithFilter(base::Time(), base::Time::Max(), data_to_remove,
                            origin_type, std::move(filter_builder));
}

void BraveEphemeralStorageServiceDelegate::RegisterFirstWindowOpenedCallback(
    base::OnceClosure callback) {
  DCHECK(callback);
#if !BUILDFLAG(IS_ANDROID)
  BrowserList::AddObserver(this);
  first_window_opened_callback_ = std::move(callback);
#else
  std::move(callback).Run();
#endif  // !BUILDFLAG(IS_ANDROID)
}

#if !BUILDFLAG(IS_ANDROID)
void BraveEphemeralStorageServiceDelegate::OnBrowserAdded(Browser* browser) {
  if (browser->profile() != Profile::FromBrowserContext(context_)) {
    return;
  }

  if (first_window_opened_callback_) {
    std::move(first_window_opened_callback_).Run();
  }

  // No need to observe anymore.
  BrowserList::RemoveObserver(this);
}
#endif

}  // namespace ephemeral_storage
