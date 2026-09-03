/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ephemeral_storage/ephemeral_storage_service.h"

#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/functional/callback_forward.h"
#include "base/json/values_util.h"
#include "base/logging.h"
#include "base/strings/strcat.h"
#include "base/task/sequenced_task_runner.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "brave/components/ephemeral_storage/ephemeral_storage_pref_names.h"
#include "brave/components/ephemeral_storage/url_storage_checker.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/storage_partition_config.h"
#include "content/public/browser/web_contents.h"
#include "net/base/features.h"
#include "net/base/schemeful_site.h"
#include "net/base/url_util.h"
#include "url/origin.h"
#include "url/url_constants.h"

namespace ephemeral_storage {

namespace {

// Keys of a first party storage area entry stored in the
// kFirstPartyStorageOriginsToCleanup pref.
constexpr char kUrlKey[] = "u";
constexpr char kPartitionDomainKey[] = "pd";
constexpr char kPartitionNameKey[] = "pn";
// Time of the last tab close. Entries stored by older versions don't have it
// and are always treated as expired.
constexpr char kClosedAtKey[] = "t";

GURL GetFirstPartyStorageURL(const std::string& ephemeral_domain) {
  return GURL(base::StrCat({url::kHttpsScheme, "://", ephemeral_domain}));
}

base::Value GetFirstPartyStorageValueToCleanup(
    const GURL& url,
    const content::StoragePartitionConfig& storage_partition_config) {
  return base::Value(
      base::DictValue()
          .Set(kUrlKey, url.spec())
          .Set(kPartitionDomainKey,
               storage_partition_config.partition_domain())
          .Set(kPartitionNameKey, storage_partition_config.partition_name())
          .Set(kClosedAtKey, base::TimeToValue(base::Time::Now())));
}

// Matches a stored entry by the storage area identity only, ignoring the close
// time.
bool IsSameFirstPartyStorageArea(
    const base::Value& value,
    const GURL& url,
    const content::StoragePartitionConfig& storage_partition_config) {
  if (value.is_string()) {
    // Entries stored by older versions used a bare url spec for the default
    // storage partition.
    return storage_partition_config.is_default() &&
           value.GetString() == url.spec();
  }
  const base::DictValue* dict = value.GetIfDict();
  if (!dict) {
    return false;
  }
  const std::string* url_spec = dict->FindString(kUrlKey);
  const std::string* partition_domain = dict->FindString(kPartitionDomainKey);
  const std::string* partition_name = dict->FindString(kPartitionNameKey);
  return url_spec && partition_domain && partition_name &&
         *url_spec == url.spec() &&
         *partition_domain == storage_partition_config.partition_domain() &&
         *partition_name == storage_partition_config.partition_name();
}

const base::Value* FindFirstPartyStorageArea(
    const base::ListValue& list,
    const GURL& url,
    const content::StoragePartitionConfig& storage_partition_config) {
  for (const base::Value& value : list) {
    if (IsSameFirstPartyStorageArea(value, url, storage_partition_config)) {
      return &value;
    }
  }
  return nullptr;
}

void EraseFirstPartyStorageArea(
    base::ListValue& list,
    const GURL& url,
    const content::StoragePartitionConfig& storage_partition_config) {
  list.EraseIf([&](const base::Value& value) {
    return IsSameFirstPartyStorageArea(value, url, storage_partition_config);
  });
}

void UpsertFirstPartyStorageOriginsToCleanup(ScopedListPrefUpdate& pref_update, const GURL& url,
    const content::StoragePartitionConfig& storage_partition_config) {
    EraseFirstPartyStorageArea(*pref_update, url, storage_partition_config);
    pref_update->Append(
        GetFirstPartyStorageValueToCleanup(url, storage_partition_config));
}

// Returns true if the keepalive that was pending when the area was stored has
// already elapsed, i.e. using the area again should no longer cancel the queued
// cleanup.
bool IsFirstPartyStorageAreaKeepAliveExpired(const base::Value& value,
                                             base::TimeDelta keep_alive) {
  const base::DictValue* dict = value.GetIfDict();
  const std::optional<base::Time> closed_at =
      dict ? base::ValueToTime(dict->Find(kClosedAtKey)) : std::nullopt;
  if (!closed_at) {
    return true;
  }
  const base::TimeDelta elapsed = base::Time::Now() - *closed_at;
  LOG(INFO) << "[SHRED] IsFirstPartyStorageAreaKeepAliveExpired closed_at:" << closed_at.value() << " elapsed:" << elapsed.InSeconds() << " keep_alive:" << keep_alive <<  " elapsed >= keep_alive:" << (elapsed >= keep_alive) << " dict:" << (dict ? dict->DebugString() : "n/a");
  // A backwards clock jump is treated as an expired keepalive.
  return elapsed.is_negative() || elapsed >= keep_alive;
}

std::optional<std::pair<GURL, content::StoragePartitionConfig>>
GetFirstPartyStorageURLAndStoragePartitionConfig(
    const base::Value& value,
    content::BrowserContext* browser_context) {
  // Support old format
  if (value.is_string()) {
    return std::make_pair(
        GURL(value.GetString()),
        content::StoragePartitionConfig::CreateDefault(browser_context));
  }
  if (!value.is_dict()) {
    return std::nullopt;
  }
  const auto& dict = value.GetDict();
  const std::string* url_spec = dict.FindString(kUrlKey);
  const std::string* partition_domain = dict.FindString(kPartitionDomainKey);
  const std::string* partition_name = dict.FindString(kPartitionNameKey);
  if (!url_spec || !partition_domain || !partition_name) {
    return std::nullopt;
  }
  if (partition_domain->empty()) {
    // The default storage partition is stored with empty partition values,
    // StoragePartitionConfig::Create() doesn't accept them.
    return std::make_pair(
        GURL(*url_spec),
        content::StoragePartitionConfig::CreateDefault(browser_context));
  }
  return std::make_pair(
      GURL(*url_spec),
      content::StoragePartitionConfig::Create(
          browser_context, *partition_domain, *partition_name, false));
}

}  // namespace

EphemeralStorageService::EphemeralStorageService(
    content::BrowserContext* context,
    HostContentSettingsMap* host_content_settings_map,
    std::unique_ptr<EphemeralStorageServiceDelegate> delegate)
    : context_(context),
      host_content_settings_map_(host_content_settings_map),
      delegate_(std::move(delegate)),
      prefs_(user_prefs::UserPrefs::Get(context_)),
      update_fp_storage_origins_to_cleanup_(base::BindRepeating(&UpsertFirstPartyStorageOriginsToCleanup)) {
  DCHECK(context_);
  DCHECK(host_content_settings_map_);
  DCHECK(delegate_);
  DCHECK(prefs_);

  tld_ephemeral_area_keep_alive_ = base::Seconds(
      net::features::kBraveEphemeralStorageKeepAliveTimeInSeconds.Get());

  RegisterFirstWindowOpenedCallback(base::BindOnce(
      &EphemeralStorageService::CleanupOnStartup,
      weak_ptr_factory_.GetWeakPtr()));
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

  // Reset delegate early to ensure proper cleanup order.
  // This prevents the delegate from being destroyed later when dependent
  // services (like HistoryService) may have already been shut down.
  delegate_.reset();
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
  FirstPartyStorageAreaInUse(ephemeral_domain, storage_partition_config);
}

void EphemeralStorageService::TLDEphemeralLifetimeDestroyed(
    const std::string& ephemeral_domain,
    const content::StoragePartitionConfig& storage_partition_config,
    bool shields_disabled_on_one_of_hosts,
    StorageCleanupMode cleanup_mode) {
  DVLOG(1) << __func__ << " " << ephemeral_domain << " "
           << storage_partition_config;
  const GURL url(GetFirstPartyStorageURL(ephemeral_domain));
  const auto auto_shred_mode = delegate_->GetAutoShredMode(url);

  // We should clean up the browsing history only if shred for browsing history
  // is enabled and the shred operation has been started manually or by the auto
  // shred feature.
  const bool cleanup_browsing_history_for_tld =
      (cleanup_mode == StorageCleanupMode::kImmediateShred ||
       (auto_shred_mode.has_value() &&
        auto_shred_mode.value() !=
            brave_shields::mojom::AutoShredMode::NEVER)) &&
      delegate_->IsShredBrowsingHistoryEnabled();

  const TLDEphemeralAreaKey key(ephemeral_domain, storage_partition_config);
  const bool cleanup_tld_ephemeral_area =
      !shields_disabled_on_one_of_hosts ||
      cleanup_mode != StorageCleanupMode::kDefault;
  const bool cleanup_first_party_storage_area =
      FirstPartyStorageAreaNotInUse(ephemeral_domain, storage_partition_config,
                                    shields_disabled_on_one_of_hosts,
                                    auto_shred_mode) ||
      cleanup_mode != StorageCleanupMode::kDefault;

  if (cleanup_mode == StorageCleanupMode::kOnExitShred &&
      cleanup_first_party_storage_area && auto_shred_mode.has_value() &&
      auto_shred_mode.value() ==
          brave_shields::mojom::AutoShredMode::APP_EXIT) {
    // In case of APP_EXIT mode we need to force commit the prefs right away to
    // make sure that they are saved before application exit.
    prefs_->CommitPendingWrite();
    return;
  }

  if (cleanup_mode != StorageCleanupMode::kDefault ||
      base::FeatureList::IsEnabled(
          net::features::kBraveEphemeralStorageKeepAlive)) {
    auto cleanup_timer = std::make_unique<base::OneShotTimer>();
    cleanup_timer->Start(
        FROM_HERE,
        cleanup_mode != StorageCleanupMode::kDefault
            ? base::Milliseconds(500)
            : tld_ephemeral_area_keep_alive_,
        base::BindOnce(&EphemeralStorageService::CleanupTLDEphemeralAreaByTimer,
                       weak_ptr_factory_.GetWeakPtr(), key,
                       cleanup_tld_ephemeral_area,
                       cleanup_first_party_storage_area,
                       cleanup_browsing_history_for_tld));
    tld_ephemeral_areas_to_cleanup_.emplace(key, std::move(cleanup_timer));
  } else {
    CleanupTLDEphemeralArea(key, cleanup_tld_ephemeral_area,
                            cleanup_first_party_storage_area,
                            cleanup_browsing_history_for_tld);
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

#if BUILDFLAG(IS_ANDROID)
void EphemeralStorageService::TriggerCurrentAppStateNotification() {
  // Register again, as on Android the EphemeralStorageService may remain alive
  // across multiple app states, requiring the callback to be re-registered.
  RegisterFirstWindowOpenedCallback(base::BindOnce(
      &EphemeralStorageService::CleanupOnStartup,
      weak_ptr_factory_.GetWeakPtr()));

  // For real cold start of the application we should not update saved time for
  // items saved in the kFirstPartyStorageOriginsToCleanup
  SetUpdateFirstPartyStorageOriginToCleanUp(true);
  delegate_->TriggerCurrentAppStateNotification();
  SetUpdateFirstPartyStorageOriginToCleanUp(false);
}
#endif  // BUILDFLAG(IS_ANDROID)

void EphemeralStorageService::CleanupTLDFirstPartyStorage(
    const GURL& url,
    const content::StoragePartitionConfig& storage_partition_config,
    const bool enforced_by_user) {
  if (!base::FeatureList::IsEnabled(
          brave_shields::features::kBraveShredFeature)) {
    return;
  }

  if (!enforced_by_user &&
      delegate_->IsShieldsDisabledOnAnyHostMatchingDomainOf(url)) {
    // Do not start auto shred if shields is disabled on any host matching the
    // domain or ephemeral_domain is empty.
    return;
  }

  const auto ephemeral_domain = net::URLToEphemeralStorageDomain(url);
  delegate_->PrepareTabsForFirstPartyStorageCleanup(
      {std::move(ephemeral_domain)}, enforced_by_user);
}

void EphemeralStorageService::FirstPartyStorageAreaInUse(
    const std::string& ephemeral_domain,
    const content::StoragePartitionConfig& storage_partition_config) {
  if (!base::FeatureList::IsEnabled(
          net::features::kBraveForgetFirstPartyStorage) &&
      !base::FeatureList::IsEnabled(
          net::features::kThirdPartyStoragePartitioning)) {
    return;
  }

  if (context_->IsOffTheRecord()) {
    return;
  }

  const GURL url(GetFirstPartyStorageURL(ephemeral_domain));
  const auto auto_shred_mode = delegate_->GetAutoShredMode(url);
  if (auto_shred_mode.has_value() &&
      auto_shred_mode.value() ==
          brave_shields::mojom::AutoShredMode::APP_EXIT) {
    LOG(INFO) << "[SHRED] EphemeralStorageService::FirstPartyStorageAreaInUse #100 auto_shred_mode:"
              << (auto_shred_mode.has_value()
                      ? static_cast<int>(auto_shred_mode.value())
                      : -1)
              << " url:" << url;
    return;
  }

  bool keep_alive_expired = false;
  ScopedListPrefUpdate pref_update(prefs_, kFirstPartyStorageOriginsToCleanup);
  if (const base::Value* queued_area = FindFirstPartyStorageArea(
          *pref_update, url, storage_partition_config)) {
    keep_alive_expired = IsFirstPartyStorageAreaKeepAliveExpired(
        *queued_area, tld_ephemeral_area_keep_alive_);
    LOG(INFO) << "[SHRED] EphemeralStorageService::FirstPartyStorageAreaInUse #200 auto_shred_mode:"
              << (auto_shred_mode.has_value()
                      ? static_cast<int>(auto_shred_mode.value())
                      : -1)
              << " url:" << url;
  }

  if (!keep_alive_expired) {
    LOG(INFO) << "[SHRED] EphemeralStorageService::FirstPartyStorageAreaInUse "
                 "#300 auto_shred_mode:"
              << (auto_shred_mode.has_value()
                      ? static_cast<int>(auto_shred_mode.value())
                      : -1)
              << " url:" << url;
    // Make sure to cancel the scheduled cleanup for this area.
    EraseFirstPartyStorageArea(*pref_update, url, storage_partition_config);
    EraseFirstPartyStorageArea(first_party_storage_areas_to_cleanup_on_startup_,
                               url, storage_partition_config);
  }
}

bool EphemeralStorageService::FirstPartyStorageAreaNotInUse(
    const std::string& ephemeral_domain,
    const content::StoragePartitionConfig& storage_partition_config,
    bool shields_disabled_on_one_of_hosts,
    const std::optional<brave_shields::mojom::AutoShredMode>& auto_shred_mode) {
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

  const auto forgetful_browser_enabled =
      !auto_shred_mode.has_value() &&
      host_content_settings_map_->GetContentSetting(
          url, url, ContentSettingsType::BRAVE_REMEMBER_1P_STORAGE) ==
          CONTENT_SETTING_BLOCK;

  const bool auto_shred_mode_enabled =
      auto_shred_mode.has_value() &&
      (auto_shred_mode.value() ==
           brave_shields::mojom::AutoShredMode::LAST_TAB_CLOSED ||
       auto_shred_mode.value() ==
           brave_shields::mojom::AutoShredMode::APP_EXIT);

  if (!forgetful_browser_enabled && !auto_shred_mode_enabled) {
    return false;
  }

  if (!context_->IsOffTheRecord()) {
    ScopedListPrefUpdate pref_update(prefs_,
                                     kFirstPartyStorageOriginsToCleanup);
LOG(INFO) << "[SHRED] EphemeralStorageService::FirstPartyStorageAreaNotInUse "
                 "#300 auto_shred_mode:"
              << (auto_shred_mode.has_value()
                      ? static_cast<int>(auto_shred_mode.value())
                      : -1)
              << " url:" << url;
    // Replace a possibly stale entry to store the actual close time.
    update_fp_storage_origins_to_cleanup_.Run(pref_update, url, storage_partition_config);
  }
  return true;
}

void EphemeralStorageService::CleanupTLDEphemeralAreaByTimer(
    const TLDEphemeralAreaKey& key,
    bool cleanup_tld_ephemeral_area,
    bool cleanup_first_party_storage_area,
    bool cleanup_browsing_history_for_tld) {
  DVLOG(1) << __func__ << " " << key.first << " " << key.second;
  tld_ephemeral_areas_to_cleanup_.erase(key);
  CleanupTLDEphemeralArea(key, cleanup_tld_ephemeral_area,
                          cleanup_first_party_storage_area,
                          cleanup_browsing_history_for_tld);
}

void EphemeralStorageService::CleanupTLDEphemeralArea(
    const TLDEphemeralAreaKey& key,
    bool cleanup_tld_ephemeral_area,
    bool cleanup_first_party_storage_area,
    bool cleanup_browsing_history_for_tld) {
  DVLOG(1) << __func__ << " " << key.first << " " << key.second;
  if (cleanup_tld_ephemeral_area) {
    delegate_->CleanupTLDEphemeralArea(key);
  }
  fpes_tokens_.erase(key.first);
  if (cleanup_first_party_storage_area) {
    CleanupFirstPartyStorageArea(key);
  }
  if (cleanup_browsing_history_for_tld) {
    delegate_->CleanupTLDBrowsingHistory(key);
  }
  for (auto& observer : observer_list_) {
    observer.OnCleanupTLDEphemeralArea(key);
  }
}

void EphemeralStorageService::CleanupFirstPartyStorageArea(
    const TLDEphemeralAreaKey& key) {
  DVLOG(1) << __func__ << " " << key.first << " " << key.second;
  delegate_->CleanupFirstPartyStorageArea(key);
  if (!context_->IsOffTheRecord()) {
    ScopedListPrefUpdate pref_update(prefs_,
                                     kFirstPartyStorageOriginsToCleanup);
    EraseFirstPartyStorageArea(*pref_update, GetFirstPartyStorageURL(key.first),
                               key.second);
  }
}

void EphemeralStorageService::CleanupPendingFirstPartyStorageArea(
    const GURL& url,
    const content::StoragePartitionConfig& storage_partition_config,
    const std::optional<brave_shields::mojom::AutoShredMode>& auto_shred_mode,
  base::OnceClosure callback) {
  DVLOG(1) << __func__ << " " << url << " " << storage_partition_config;
  const TLDEphemeralAreaKey key(std::string(url.host()),
                                storage_partition_config);
  delegate_->CleanupFirstPartyStorageArea(key, std::move(callback));

  if (auto_shred_mode.has_value() &&
      auto_shred_mode.value() != brave_shields::mojom::AutoShredMode::NEVER &&
      delegate_->IsShredBrowsingHistoryEnabled()) {
    // We should clean up the browsing history if shred for browsing history is
    // enabled.
    delegate_->CleanupTLDBrowsingHistory(key);
  }
}

void EphemeralStorageService::CleanupOnStartup() {
  DCHECK(!context_->IsOffTheRecord());
  first_party_storage_areas_to_cleanup_on_startup_ =
      prefs_->GetList(kFirstPartyStorageOriginsToCleanup).Clone();

  LOG(INFO) << "[SHRED] EphemeralStorageService::CleanupOnStartup list:"
            << first_party_storage_areas_to_cleanup_on_startup_.DebugString();

  ScopedListPrefUpdate pref_update(prefs_, kFirstPartyStorageOriginsToCleanup);

  std::vector<std::pair<std::string, base::OnceClosure>> ephemeral_domains;
  for (base::Value& area_to_cleanup :
       first_party_storage_areas_to_cleanup_on_startup_) {
    pref_update->EraseValue(area_to_cleanup);
    if (!IsFirstPartyStorageAreaKeepAliveExpired(
            area_to_cleanup, tld_ephemeral_area_keep_alive_)) {
LOG(INFO) << "[SHRED] EphemeralStorageService::CleanupOnStartup #100";
      continue;
    }

    const auto url_and_storage_partition_config =
        GetFirstPartyStorageURLAndStoragePartitionConfig(area_to_cleanup,
                                                         context_);
    if (!url_and_storage_partition_config) {
LOG(INFO) << "[SHRED] EphemeralStorageService::CleanupOnStartup #200";
      continue;
    }

    const auto& [url, storage_partition_config] =
        *url_and_storage_partition_config;
    if (!url.is_valid()) {
LOG(INFO) << "[SHRED] EphemeralStorageService::CleanupOnStartup #300";
      continue;
    }
    LOG(INFO) << "[SHRED] EphemeralStorageService::CleanupOnStartup area_to_cleanup:" << area_to_cleanup.DebugString();

    CleanupPendingFirstPartyStorageArea(url, storage_partition_config,
            delegate_->GetAutoShredMode(url), 
            base::BindOnce(
            &EphemeralStorageServiceDelegate::ReloadTabIfMatchingEphemeralDomain,
            delegate_->AsWeakPtr(), net::URLToEphemeralStorageDomain(url)));
  }

  first_party_storage_areas_to_cleanup_on_startup_.clear();
}

void EphemeralStorageService::RegisterFirstWindowOpenedCallback(
    base::OnceClosure callback) {
  if (!base::FeatureList::IsEnabled(
          net::features::kBraveForgetFirstPartyStorage) ||
      context_->IsOffTheRecord()) {
    return;
  }

  delegate_->RegisterFirstWindowOpenedCallback(std::move(callback));
}

void EphemeralStorageService::SetUpdateFirstPartyStorageOriginToCleanUp(bool do_nothing) {
  if (do_nothing) {
    update_fp_storage_origins_to_cleanup_ = base::DoNothing();
  } else {
    update_fp_storage_origins_to_cleanup_ = base::BindRepeating(&UpsertFirstPartyStorageOriginsToCleanup);
  }
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
  DCHECK(first_party_storage_areas_to_cleanup_on_startup_.empty());
  return timers.size() + first_party_storage_areas_to_cleanup_count;
}

}  // namespace ephemeral_storage
