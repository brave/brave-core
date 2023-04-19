/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ephemeral_storage/ephemeral_storage_service.h"

#include <utility>

#include "base/task/sequenced_task_runner.h"
#include "base/timer/timer.h"
#include "brave/components/ephemeral_storage/ephemeral_storage_pref_names.h"
#include "brave/components/ephemeral_storage/url_storage_checker.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browsing_data_filter_builder.h"
#include "content/public/browser/browsing_data_remover.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/storage_partition.h"
#include "net/base/features.h"
#include "net/base/schemeful_site.h"
#include "url/origin.h"
#include "url/url_constants.h"

namespace ephemeral_storage {

namespace {

bool IsOriginAcceptableForFirstPartyStorageCleanup(const url::Origin& origin) {
  return !origin.opaque() && (origin.scheme() == url::kHttpScheme ||
                              origin.scheme() == url::kHttpsScheme);
}

}  // namespace

EphemeralStorageService::EphemeralStorageService(
    content::BrowserContext* context,
    HostContentSettingsMap* host_content_settings_map)
    : context_(context), host_content_settings_map_(host_content_settings_map) {
  DCHECK(context_);
  DCHECK(host_content_settings_map_);

  if (base::FeatureList::IsEnabled(
          net::features::kBraveForgetFirstPartyStorage)) {
    first_party_storage_areas_keep_alive_ = base::Seconds(
        net::features::kBraveForgetFirstPartyStorageKeepAliveTimeInSeconds
            .Get());
    CleanupFirstPartyStorageAreasOnStartup();
  }
}

EphemeralStorageService::~EphemeralStorageService() = default;

void EphemeralStorageService::Shutdown() {
  for (const auto& pattern : patterns_to_cleanup_on_shutdown_) {
    host_content_settings_map_->SetContentSettingCustomScope(
        pattern, ContentSettingsPattern::Wildcard(),
        ContentSettingsType::COOKIES, CONTENT_SETTING_DEFAULT);
  }
  weak_ptr_factory_.InvalidateWeakPtrs();
}

base::WeakPtr<EphemeralStorageService> EphemeralStorageService::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

void EphemeralStorageService::CanEnable1PESForUrl(
    const GURL& url,
    base::OnceCallback<void(bool can_enable_1pes)> callback) const {
  if (!IsDefaultCookieSetting(url)) {
    std::move(callback).Run(false);
    return;
  }

  auto site_instance = content::SiteInstance::CreateForURL(context_, url);
  auto* storage_partition = context_->GetStoragePartition(site_instance.get());
  DCHECK(storage_partition);
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE,
      base::BindOnce(&UrlStorageChecker::StartCheck,
                     base::MakeRefCounted<UrlStorageChecker>(
                         *storage_partition, url, std::move(callback))));
}

void EphemeralStorageService::Set1PESEnabledForUrl(const GURL& url,
                                                   bool enable) {
  auto pattern = ContentSettingsPattern::FromURLNoWildcard(url);
  if (enable) {
    patterns_to_cleanup_on_shutdown_.insert(pattern);
  } else {
    patterns_to_cleanup_on_shutdown_.erase(pattern);
  }
  host_content_settings_map_->SetContentSettingCustomScope(
      pattern, ContentSettingsPattern::Wildcard(), ContentSettingsType::COOKIES,
      enable ? CONTENT_SETTING_SESSION_ONLY : CONTENT_SETTING_DEFAULT);
}

bool EphemeralStorageService::Is1PESEnabledForUrl(const GURL& url) const {
  return host_content_settings_map_->GetContentSetting(
             url, url, ContentSettingsType::COOKIES) ==
         CONTENT_SETTING_SESSION_ONLY;
}

void EphemeralStorageService::Enable1PESForUrlIfPossible(
    const GURL& url,
    base::OnceCallback<void(bool)> on_ready) {
  CanEnable1PESForUrl(
      url,
      base::BindOnce(&EphemeralStorageService::OnCanEnable1PESForUrl,
                     weak_ptr_factory_.GetWeakPtr(), url, std::move(on_ready)));
}

void EphemeralStorageService::OnCanEnable1PESForUrl(
    const GURL& url,
    base::OnceCallback<void(bool)> on_ready,
    bool can_enable_1pes) {
  if (can_enable_1pes) {
    Set1PESEnabledForUrl(url, true);
  }
  std::move(on_ready).Run(can_enable_1pes);
}

bool EphemeralStorageService::IsDefaultCookieSetting(const GURL& url) const {
  ContentSettingsForOneType settings;
  host_content_settings_map_->GetSettingsForOneType(
      ContentSettingsType::COOKIES, &settings);

  for (const auto& setting : settings) {
    if (setting.primary_pattern.Matches(url) &&
        setting.secondary_pattern.Matches(url)) {
      return setting.source == "default";
    }
  }

  return true;
}

void EphemeralStorageService::FirstPartyStorageAreaInUse(
    const url::Origin& origin) {
  if (!base::FeatureList::IsEnabled(
          net::features::kBraveForgetFirstPartyStorage)) {
    return;
  }

  if (!IsOriginAcceptableForFirstPartyStorageCleanup(origin)) {
    return;
  }

  if (!first_party_storage_areas_to_cleanup_.erase(origin)) {
    return;
  }

  ScopedListPrefUpdate pref_update(user_prefs::UserPrefs::Get(context_),
                                   kFirstPartyStorageOriginsToCleanup);
  pref_update->EraseValue(base::Value(origin.Serialize()));
}

void EphemeralStorageService::FirstPartyStorageAreaNotInUse(
    const url::Origin& origin) {
  if (!base::FeatureList::IsEnabled(
          net::features::kBraveForgetFirstPartyStorage)) {
    return;
  }

  if (!IsOriginAcceptableForFirstPartyStorageCleanup(origin)) {
    return;
  }

  const auto& url = origin.GetURL();
  if (host_content_settings_map_->GetContentSetting(
          url, url, ContentSettingsType::BRAVE_REMEMBER_1P_STORAGE) !=
      CONTENT_SETTING_BLOCK) {
    return;
  }

  auto cleanup_timer = std::make_unique<base::OneShotTimer>();
  cleanup_timer->Start(
      FROM_HERE, first_party_storage_areas_keep_alive_,
      base::BindOnce(
          &EphemeralStorageService::CleanupFirstPartyStorageAreaByTimer,
          weak_ptr_factory_.GetWeakPtr(), origin));
  first_party_storage_areas_to_cleanup_.emplace(origin,
                                                std::move(cleanup_timer));
  ScopedListPrefUpdate pref_update(user_prefs::UserPrefs::Get(context_),
                                   kFirstPartyStorageOriginsToCleanup);
  pref_update->Append(base::Value(origin.Serialize()));
}

void EphemeralStorageService::CleanupFirstPartyStorageAreasOnStartup() {
  ScopedListPrefUpdate urls_to_cleanup(user_prefs::UserPrefs::Get(context_),
                                       kFirstPartyStorageOriginsToCleanup);
  for (const auto& url_to_cleanup : urls_to_cleanup.Get()) {
    const auto* url_string = url_to_cleanup.GetIfString();
    if (!url_string) {
      continue;
    }
    const GURL url(*url_string);
    if (!url.is_valid()) {
      continue;
    }
    CleanupFirstPartyStorageArea(url::Origin::Create(url));
  }
  urls_to_cleanup->clear();
}

void EphemeralStorageService::CleanupFirstPartyStorageAreaByTimer(
    const url::Origin& origin) {
  DCHECK(base::Contains(first_party_storage_areas_to_cleanup_, origin));

  CleanupFirstPartyStorageArea(origin);

  first_party_storage_areas_to_cleanup_.erase(origin);
  ScopedListPrefUpdate pref_update(user_prefs::UserPrefs::Get(context_),
                                   kFirstPartyStorageOriginsToCleanup);
  pref_update->EraseValue(base::Value(origin.Serialize()));
}

void EphemeralStorageService::CleanupFirstPartyStorageArea(
    const url::Origin& origin) {
  DCHECK(base::FeatureList::IsEnabled(
      net::features::kBraveForgetFirstPartyStorage));
  content::BrowsingDataRemover* remover = context_->GetBrowsingDataRemover();
  content::BrowsingDataRemover::DataType data_to_remove =
      content::BrowsingDataRemover::DATA_TYPE_DOM_STORAGE;
  content::BrowsingDataRemover::OriginType origin_type =
      content::BrowsingDataRemover::ORIGIN_TYPE_UNPROTECTED_WEB |
      content::BrowsingDataRemover::ORIGIN_TYPE_PROTECTED_WEB;
  auto filter_builder = content::BrowsingDataFilterBuilder::Create(
      content::BrowsingDataFilterBuilder::Mode::kDelete);
  filter_builder->AddOrigin(origin);
  remover->RemoveWithFilter(base::Time(), base::Time::Max(), data_to_remove,
                            origin_type, std::move(filter_builder));

  const auto& url = net::SchemefulSite(origin).GetURL();
  auto cookie_deletion_filter = network::mojom::CookieDeletionFilter::New();
  cookie_deletion_filter->including_domains.emplace({url.host()});

  auto site_instance = content::SiteInstance::CreateForURL(context_, url);
  auto* storage_partition = context_->GetStoragePartition(site_instance.get());
  if (storage_partition) {
    storage_partition->GetCookieManagerForBrowserProcess()->DeleteCookies(
        std::move(cookie_deletion_filter), base::NullCallback());
  }
}

}  // namespace ephemeral_storage
