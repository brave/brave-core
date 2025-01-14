/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ephemeral_storage/ephemeral_storage_service.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/strings/strcat.h"
#include "base/task/sequenced_task_runner.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "brave/components/ephemeral_storage/ephemeral_storage_pref_names.h"
#include "brave/components/ephemeral_storage/url_storage_checker.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/storage_partition_config.h"
#include "net/base/features.h"
#include "net/base/schemeful_site.h"
#include "net/base/url_util.h"
#include "url/origin.h"
#include "url/url_constants.h"

namespace ephemeral_storage {

namespace {

GURL GetFirstPartyStorageURL(const std::string& ephemeral_domain) {
  return GURL(base::StrCat({url::kHttpsScheme, "://", ephemeral_domain}));
}

}  // namespace

EphemeralStorageService::EphemeralStorageService(
    content::BrowserContext* context,
    HostContentSettingsMap* host_content_settings_map,
    std::unique_ptr<EphemeralStorageServiceDelegate> delegate)
    : context_(context),
      host_content_settings_map_(host_content_settings_map),
      delegate_(std::move(delegate)),
      prefs_(user_prefs::UserPrefs::Get(context_)) {
  DCHECK(context_);
  DCHECK(host_content_settings_map_);
  DCHECK(delegate_);
  DCHECK(prefs_);

  tld_ephemeral_area_keep_alive_ = base::Seconds(
      net::features::kBraveEphemeralStorageKeepAliveTimeInSeconds.Get());

  if (base::FeatureList::IsEnabled(
          net::features::kBraveForgetFirstPartyStorage) &&
      !context_->IsOffTheRecord()) {
    delegate_->RegisterFirstWindowOpenedCallback(
        base::BindOnce(&EphemeralStorageService::
                           ScheduleFirstPartyStorageAreasCleanupOnStartup,
                       weak_ptr_factory_.GetWeakPtr()));
  }
}

EphemeralStorageService::~EphemeralStorageService() = default;

void EphemeralStorageService::Shutdown() {
  for (const auto& pattern : patterns_to_cleanup_on_shutdown_) {
    host_content_settings_map_->SetContentSettingCustomScope(
        pattern, ContentSettingsPattern::Wildcard(),
        ContentSettingsType::COOKIES, CONTENT_SETTING_DEFAULT);
  }
  observer_list_.Clear();
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
  content_settings::SettingInfo settings_info;
  return host_content_settings_map_->GetContentSetting(
             url, url, ContentSettingsType::COOKIES, &settings_info) ==
             CONTENT_SETTING_SESSION_ONLY &&
         !settings_info.primary_pattern.MatchesAllHosts();
}

void EphemeralStorageService::Enable1PESForUrlIfPossible(
    const GURL& url,
    base::OnceCallback<void(bool)> on_ready) {
  CanEnable1PESForUrl(
      url,
      base::BindOnce(&EphemeralStorageService::OnCanEnable1PESForUrl,
                     weak_ptr_factory_.GetWeakPtr(), url, std::move(on_ready)));
}

std::optional<base::UnguessableToken> EphemeralStorageService::Get1PESToken(
    const url::Origin& origin) {
  const GURL url(origin.GetURL());
  const std::string ephemeral_storage_domain =
      net::URLToEphemeralStorageDomain(url);
  std::optional<base::UnguessableToken> token;
  if (Is1PESEnabledForUrl(url)) {
    auto token_it = fpes_tokens_.find(ephemeral_storage_domain);
    if (token_it != fpes_tokens_.end()) {
      return token_it->second;
    }
    token = base::UnguessableToken::Create();
    fpes_tokens_[ephemeral_storage_domain] = *token;
  }
  return token;
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
  ContentSettingsForOneType settings =
      host_content_settings_map_->GetSettingsForOneType(
          ContentSettingsType::COOKIES);

  for (const auto& setting : settings) {
    if (setting.primary_pattern.Matches(url) &&
        setting.secondary_pattern.Matches(url)) {
      return setting.source == content_settings::ProviderType::kDefaultProvider;
    }
  }

  return true;
}

void EphemeralStorageService::TLDEphemeralLifetimeCreated(
    const std::string& ephemeral_domain,
    const content::StoragePartitionConfig& storage_partition_config) {
  DVLOG(1) << __func__ << " " << ephemeral_domain << " "
           << storage_partition_config;
  const TLDEphemeralAreaKey key(ephemeral_domain, storage_partition_config);
  tld_ephemeral_areas_to_cleanup_.erase(key);
  FirstPartyStorageAreaInUse(ephemeral_domain);
}

void EphemeralStorageService::TLDEphemeralLifetimeDestroyed(
    const std::string& ephemeral_domain,
    const content::StoragePartitionConfig& storage_partition_config,
    bool shields_disabled_on_one_of_hosts) {
  DVLOG(1) << __func__ << " " << ephemeral_domain << " "
           << storage_partition_config;
  const TLDEphemeralAreaKey key(ephemeral_domain, storage_partition_config);
  const bool cleanup_tld_ephemeral_area = !shields_disabled_on_one_of_hosts;
  const bool cleanup_first_party_storage_area = FirstPartyStorageAreaNotInUse(
      ephemeral_domain, shields_disabled_on_one_of_hosts);

  if (base::FeatureList::IsEnabled(
          net::features::kBraveEphemeralStorageKeepAlive)) {
    auto cleanup_timer = std::make_unique<base::OneShotTimer>();
    cleanup_timer->Start(
        FROM_HERE, tld_ephemeral_area_keep_alive_,
        base::BindOnce(&EphemeralStorageService::CleanupTLDEphemeralAreaByTimer,
                       weak_ptr_factory_.GetWeakPtr(), key,
                       cleanup_tld_ephemeral_area,
                       cleanup_first_party_storage_area));
    tld_ephemeral_areas_to_cleanup_.emplace(key, std::move(cleanup_timer));
  } else {
    CleanupTLDEphemeralArea(key, cleanup_tld_ephemeral_area,
                            cleanup_first_party_storage_area);
  }
}

void EphemeralStorageService::AddObserver(
    EphemeralStorageServiceObserver* observer) {
  observer_list_.AddObserver(observer);
}

void EphemeralStorageService::RemoveObserver(
    EphemeralStorageServiceObserver* observer) {
  observer_list_.RemoveObserver(observer);
}

void EphemeralStorageService::FirstPartyStorageAreaInUse(
    const std::string& ephemeral_domain) {
  if (!base::FeatureList::IsEnabled(
          net::features::kBraveForgetFirstPartyStorage) &&
      !base::FeatureList::IsEnabled(
          net::features::kThirdPartyStoragePartitioning)) {
    return;
  }

  if (!context_->IsOffTheRecord()) {
    base::Value url_spec(GetFirstPartyStorageURL(ephemeral_domain).spec());
    ScopedListPrefUpdate pref_update(prefs_,
                                     kFirstPartyStorageOriginsToCleanup);
    pref_update->EraseValue(url_spec);

    // Make sure to cancel the scheduled cleanup for this area.
    first_party_storage_areas_to_cleanup_on_startup_.EraseValue(url_spec);
  }
}

bool EphemeralStorageService::FirstPartyStorageAreaNotInUse(
    const std::string& ephemeral_domain,
    bool shields_disabled_on_one_of_hosts) {
  if (!base::FeatureList::IsEnabled(
          net::features::kBraveForgetFirstPartyStorage) &&
      !base::FeatureList::IsEnabled(
          net::features::kThirdPartyStoragePartitioning)) {
    return false;
  }

  const GURL url(GetFirstPartyStorageURL(ephemeral_domain));
  if (base::FeatureList::IsEnabled(
          net::features::kThirdPartyStoragePartitioning) &&
      Is1PESEnabledForUrl(url)) {
    return false;
  }

  if (shields_disabled_on_one_of_hosts) {
    // Don't cleanup first party storage if we saw a website that has shields
    // disabled.
    return false;
  }

  if (host_content_settings_map_->GetContentSetting(
          url, url, ContentSettingsType::BRAVE_REMEMBER_1P_STORAGE) !=
      CONTENT_SETTING_BLOCK) {
    return false;
  }

  if (!context_->IsOffTheRecord()) {
    ScopedListPrefUpdate pref_update(prefs_,
                                     kFirstPartyStorageOriginsToCleanup);
    pref_update->Append(base::Value(url.spec()));
  }
  return true;
}

void EphemeralStorageService::CleanupTLDEphemeralAreaByTimer(
    const TLDEphemeralAreaKey& key,
    bool cleanup_tld_ephemeral_area,
    bool cleanup_first_party_storage_area) {
  DVLOG(1) << __func__ << " " << key.first << " " << key.second;
  tld_ephemeral_areas_to_cleanup_.erase(key);
  CleanupTLDEphemeralArea(key, cleanup_tld_ephemeral_area,
                          cleanup_first_party_storage_area);
}

void EphemeralStorageService::CleanupTLDEphemeralArea(
    const TLDEphemeralAreaKey& key,
    bool cleanup_tld_ephemeral_area,
    bool cleanup_first_party_storage_area) {
  DVLOG(1) << __func__ << " " << key.first << " " << key.second;
  if (cleanup_tld_ephemeral_area) {
    delegate_->CleanupTLDEphemeralArea(key);
  }
  fpes_tokens_.erase(key.first);
  if (cleanup_first_party_storage_area) {
    CleanupFirstPartyStorageArea(key.first);
  }
  for (auto& observer : observer_list_) {
    observer.OnCleanupTLDEphemeralArea(key);
  }
}

void EphemeralStorageService::CleanupFirstPartyStorageArea(
    const std::string& ephemeral_domain) {
  DVLOG(1) << __func__ << " " << ephemeral_domain;
  delegate_->CleanupFirstPartyStorageArea(ephemeral_domain);
  if (!context_->IsOffTheRecord()) {
    base::Value url_spec(GetFirstPartyStorageURL(ephemeral_domain).spec());
    ScopedListPrefUpdate pref_update(prefs_,
                                     kFirstPartyStorageOriginsToCleanup);
    pref_update->EraseValue(url_spec);
  }
}

void EphemeralStorageService::ScheduleFirstPartyStorageAreasCleanupOnStartup() {
  DVLOG(1) << __func__;
  DCHECK(!context_->IsOffTheRecord());
  first_party_storage_areas_to_cleanup_on_startup_ =
      prefs_->GetList(kFirstPartyStorageOriginsToCleanup).Clone();

  first_party_storage_areas_startup_cleanup_timer_.Start(
      FROM_HERE,
      base::Seconds(
          net::features::
              kBraveForgetFirstPartyStorageStartupCleanupDelayInSeconds.Get()),
      base::BindOnce(
          &EphemeralStorageService::CleanupFirstPartyStorageAreasOnStartup,
          weak_ptr_factory_.GetWeakPtr()));
}

void EphemeralStorageService::CleanupFirstPartyStorageAreasOnStartup() {
  DCHECK(!context_->IsOffTheRecord());
  ScopedListPrefUpdate pref_update(prefs_, kFirstPartyStorageOriginsToCleanup);
  for (const auto& url_to_cleanup :
       first_party_storage_areas_to_cleanup_on_startup_) {
    const auto* url_string = url_to_cleanup.GetIfString();
    if (!url_string) {
      continue;
    }
    pref_update->EraseValue(url_to_cleanup);
    const GURL url(*url_string);
    if (!url.is_valid()) {
      continue;
    }
    delegate_->CleanupFirstPartyStorageArea(url.host());
  }
  first_party_storage_areas_to_cleanup_on_startup_.clear();
}

size_t EphemeralStorageService::FireCleanupTimersForTesting() {
  std::vector<base::OneShotTimer*> timers;
  for (const auto& areas_to_cleanup : tld_ephemeral_areas_to_cleanup_) {
    timers.push_back(areas_to_cleanup.second.get());
  }
  for (auto* timer : timers) {
    timer->FireNow();
  }
  const size_t first_party_storage_areas_to_cleanup_count =
      first_party_storage_areas_to_cleanup_on_startup_.size();
  if (first_party_storage_areas_startup_cleanup_timer_.IsRunning()) {
    first_party_storage_areas_startup_cleanup_timer_.FireNow();
  }
  DCHECK(first_party_storage_areas_to_cleanup_on_startup_.empty());
  return timers.size() + first_party_storage_areas_to_cleanup_count;
}

}  // namespace ephemeral_storage
