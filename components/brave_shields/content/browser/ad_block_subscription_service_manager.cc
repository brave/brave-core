// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/content/browser/ad_block_subscription_service_manager.h"

#include <memory>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

#include "base/base64url.h"
#include "base/containers/contains.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/json/json_value_converter.h"
#include "base/json/values_util.h"
#include "base/task/thread_pool.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/brave_shields/adblock/rs/src/lib.rs.h"
#include "brave/components/brave_shields/content/browser/ad_block_subscription_filters_provider.h"
#include "brave/components/brave_shields/content/browser/ad_block_subscription_service_manager_observer.h"
#include "brave/components/brave_shields/core/browser/ad_block_filters_provider_manager.h"
#include "brave/components/brave_shields/core/browser/ad_block_list_p3a.h"
#include "brave/components/brave_shields/core/common/brave_shield_constants.h"
#include "brave/components/brave_shields/core/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "crypto/sha2.h"
#include "net/base/filename_util.h"

namespace brave_shields {

constexpr uint16_t kSubscriptionDefaultExpiresHours = 7 * 24;

base::TimeDelta* g_testing_subscription_retry_interval = nullptr;

namespace {

const uint16_t kSubscriptionMaxExpiresHours = 14 * 24;
constexpr base::TimeDelta kListRetryInterval = base::Hours(1);
constexpr base::TimeDelta kListCheckInitialDelay = base::Minutes(1);

bool SkipGURLField(std::string_view value, GURL* field) {
  return true;
}

bool ParseTimeValue(const base::Value* value, base::Time* field) {
  auto time = base::ValueToTime(value);
  if (!time) {
    return false;
  }
  *field = *time;
  return true;
}

bool ParseOptionalStringField(const base::Value* value,
                              std::optional<std::string>* field) {
  if (value == nullptr) {
    *field = std::nullopt;
    return true;
  } else if (!value->is_string()) {
    return false;
  } else {
    *field = value->GetString();
    return true;
  }
}

bool ParseExpiresWithFallback(const base::Value* value, uint16_t* field) {
  if (value == nullptr) {
    *field = kSubscriptionDefaultExpiresHours;
    return true;
  } else if (!value->is_int()) {
    return false;
  } else {
    int64_t i = value->GetInt();
    if (i < 0 || i > kSubscriptionMaxExpiresHours) {
      return false;
    }
    *field = (uint16_t)i;
    return true;
  }
}

SubscriptionInfo BuildInfoFromDict(const GURL& sub_url,
                                   const base::Value::Dict& dict) {
  SubscriptionInfo info;
  base::JSONValueConverter<SubscriptionInfo> converter;
  converter.Convert(base::Value(dict.Clone()), &info);

  info.subscription_url = sub_url;

  return info;
}

const base::FilePath::CharType kSubscriptionsDir[] =
    FILE_PATH_LITERAL("FilterListSubscriptionCache");

}  // namespace

SubscriptionInfo::SubscriptionInfo() = default;
SubscriptionInfo::~SubscriptionInfo() = default;
SubscriptionInfo::SubscriptionInfo(const SubscriptionInfo&) = default;

void SubscriptionInfo::RegisterJSONConverter(
    base::JSONValueConverter<SubscriptionInfo>* converter) {
  // The `subscription_url` field is skipped, as it's not stored within the
  // JSON value and should be populated externally.
  converter->RegisterCustomField<GURL>(
      "subscription_url", &SubscriptionInfo::subscription_url, &SkipGURLField);
  converter->RegisterCustomValueField<base::Time>(
      "last_update_attempt", &SubscriptionInfo::last_update_attempt,
      &ParseTimeValue);
  converter->RegisterCustomValueField<base::Time>(
      "last_successful_update_attempt",
      &SubscriptionInfo::last_successful_update_attempt, &ParseTimeValue);
  converter->RegisterBoolField("enabled", &SubscriptionInfo::enabled);
  converter->RegisterCustomValueField<std::optional<std::string>>(
      "homepage", &SubscriptionInfo::homepage, &ParseOptionalStringField);
  converter->RegisterCustomValueField<std::optional<std::string>>(
      "title", &SubscriptionInfo::title, &ParseOptionalStringField);
  converter->RegisterCustomValueField<uint16_t>(
      "expires", &SubscriptionInfo::expires, &ParseExpiresWithFallback);
}

AdBlockSubscriptionServiceManager::AdBlockSubscriptionServiceManager(
    PrefService* local_state,
    AdBlockSubscriptionDownloadManager::DownloadManagerGetter
        download_manager_getter,
    const base::FilePath& profile_dir,
    AdBlockListP3A* list_p3a)
    : initialized_(false),
      local_state_(local_state),
      subscription_path_(profile_dir.Append(kSubscriptionsDir)),
      subscription_update_timer_(
          std::make_unique<component_updater::TimerUpdateScheduler>()),
      list_p3a_(list_p3a) {
  std::move(download_manager_getter)
      .Run(base::BindOnce(
          &AdBlockSubscriptionServiceManager::OnGetDownloadManager,
          weak_ptr_factory_.GetWeakPtr()));
}

AdBlockSubscriptionServiceManager::~AdBlockSubscriptionServiceManager() =
    default;

base::FilePath AdBlockSubscriptionServiceManager::GetSubscriptionPath(
    const GURL& sub_url) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // Subdirectories are generated by taking the SHA256 hash of the list URL
  // spec, then base64 encoding that hash. This generates paths that are:
  //     - deterministic
  //     - unique
  //     - constant length
  //     - path-safe
  //     - not too long (exactly 45 characters)
  const std::string hash = crypto::SHA256HashString(sub_url.spec());

  std::string pathsafe_hash;
  base::Base64UrlEncode(hash, base::Base64UrlEncodePolicy::INCLUDE_PADDING,
                        &pathsafe_hash);

  return subscription_path_.AppendASCII(pathsafe_hash);
}

GURL AdBlockSubscriptionServiceManager::GetListTextFileUrl(
    const GURL sub_url) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  base::FilePath cached_list_path = GetSubscriptionPath(sub_url).Append(
      brave_shields::kCustomSubscriptionListText);

  const GURL file_url = net::FilePathToFileURL(cached_list_path);

  return file_url;
}

void AdBlockSubscriptionServiceManager::OnUpdateTimer(
    component_updater::TimerUpdateScheduler::OnFinishedCallback on_finished) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!local_state_) {
    return;
  }

  subscriptions_ =
      local_state_->GetDict(prefs::kAdBlockListSubscriptions).Clone();

  for (const auto it : subscriptions_) {
    const std::string key = it.first;
    SubscriptionInfo info;
    const base::Value::Dict* list_subscription_dict =
        subscriptions_.FindDict(key);
    if (list_subscription_dict) {
      GURL sub_url(key);
      info = BuildInfoFromDict(sub_url, *list_subscription_dict);

      base::TimeDelta until_next_refresh =
          base::Hours(info.expires) -
          (base::Time::Now() - info.last_update_attempt);

      if (info.enabled &&
          ((info.last_update_attempt != info.last_successful_update_attempt) ||
           (until_next_refresh <= base::TimeDelta()))) {
        StartDownload(sub_url, false);
      }
    }
  }

  std::move(on_finished).Run();
}

void AdBlockSubscriptionServiceManager::StartDownload(const GURL& sub_url,
                                                      bool from_ui) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // The download manager is tied to the lifetime of the profile, but
  // the AdBlockSubscriptionServiceManager lives as long as the browser process
  if (download_manager_) {
    bool download_service_available =
        download_manager_->IsAvailableForDownloads();
    if (download_service_available) {
      download_manager_->StartDownload(sub_url, from_ui);
    }
  }
}

void AdBlockSubscriptionServiceManager::CreateSubscription(
    const GURL& sub_url) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (base::Contains(subscription_filters_providers_, sub_url)) {
    return;
  }

  SubscriptionInfo info;
  info.subscription_url = sub_url;
  info.last_update_attempt = base::Time();
  info.last_successful_update_attempt = base::Time();
  info.enabled = true;

  UpdateSubscriptionPrefs(sub_url, info);

  auto subscription_filters_provider =
      std::make_unique<AdBlockSubscriptionFiltersProvider>(
          local_state_,
          GetSubscriptionPath(sub_url).Append(kCustomSubscriptionListText),
          base::BindRepeating(
              &AdBlockSubscriptionServiceManager::OnListMetadata,
              weak_ptr_factory_.GetWeakPtr(), sub_url));
  subscription_filters_providers_.insert(
      std::make_pair(sub_url, std::move(subscription_filters_provider)));

  StartDownload(sub_url, true);
}

std::vector<SubscriptionInfo>
AdBlockSubscriptionServiceManager::GetSubscriptions() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  auto infos = std::vector<SubscriptionInfo>();

  for (const auto subscription : subscriptions_) {
    auto info = GetInfo(GURL(subscription.first));
    DCHECK(info);
    infos.push_back(*info);
  }

  return infos;
}

void AdBlockSubscriptionServiceManager::EnableSubscription(const GURL& sub_url,
                                                           bool enabled) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  std::optional<SubscriptionInfo> info = GetInfo(sub_url);

  DCHECK(info);

  info->enabled = enabled;

  auto it = subscription_filters_providers_.find(sub_url);
  if (enabled) {
    DCHECK(it == subscription_filters_providers_.end());
    auto subscription_filters_provider =
        std::make_unique<AdBlockSubscriptionFiltersProvider>(
            local_state_,
            GetSubscriptionPath(sub_url).Append(kCustomSubscriptionListText),
            base::BindRepeating(
                &AdBlockSubscriptionServiceManager::OnListMetadata,
                weak_ptr_factory_.GetWeakPtr(), sub_url));
    subscription_filters_provider->OnListAvailable();
    subscription_filters_providers_.insert(
        {sub_url, std::move(subscription_filters_provider)});
  } else {
    DCHECK(it != subscription_filters_providers_.end());
    subscription_filters_providers_.erase(it);
  }

  UpdateSubscriptionPrefs(sub_url, *info);
}

void AdBlockSubscriptionServiceManager::DeleteSubscription(
    const GURL& sub_url) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  auto it = subscription_filters_providers_.find(sub_url);
  if (it != subscription_filters_providers_.end()) {
    subscription_filters_providers_.erase(it);
  }
  ClearSubscriptionPrefs(sub_url);

  base::ThreadPool::PostTask(
      FROM_HERE,
      {base::MayBlock(), base::TaskPriority::BEST_EFFORT,
       base::TaskShutdownBehavior::BLOCK_SHUTDOWN},
      base::BindOnce(base::IgnoreResult(&base::DeletePathRecursively),
                     GetSubscriptionPath(sub_url)));
}

void AdBlockSubscriptionServiceManager::RefreshSubscription(const GURL& sub_url,
                                                            bool from_ui) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  StartDownload(sub_url, true);
}

void AdBlockSubscriptionServiceManager::OnGetDownloadManager(
    AdBlockSubscriptionDownloadManager* download_manager) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  download_manager_ = download_manager->AsWeakPtr();
  // base::Unretained is ok here because AdBlockSubscriptionServiceManager will
  // outlive AdBlockSubscriptionDownloadManager
  download_manager_->set_subscription_path_callback(base::BindRepeating(
      &AdBlockSubscriptionServiceManager::GetSubscriptionPath,
      base::Unretained(this)));
  download_manager_->set_on_download_succeeded_callback(base::BindRepeating(
      &AdBlockSubscriptionServiceManager::OnSubscriptionDownloaded,
      base::Unretained(this)));
  download_manager_->set_on_download_failed_callback(base::BindRepeating(
      &AdBlockSubscriptionServiceManager::OnSubscriptionDownloadFailure,
      base::Unretained(this)));

  download_manager_->CancelAllPendingDownloads();
  LoadSubscriptionServices();

  subscription_update_timer_->Schedule(
      kListCheckInitialDelay, kListRetryInterval,
      base::BindRepeating(&AdBlockSubscriptionServiceManager::OnUpdateTimer,
                          weak_ptr_factory_.GetWeakPtr()),
      base::DoNothing());
}

void AdBlockSubscriptionServiceManager::OnListMetadata(
    const GURL& sub_url,
    const adblock::FilterListMetadata& metadata) {
  // The engine will have loaded new list metadata; read it and update local
  // preferences with the new values.

  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  std::optional<SubscriptionInfo> info = GetInfo(sub_url);

  if (!info) {
    return;
  }

  // Title can only be set once - only set it if an existing title does not
  // exist
  if (!info->title && metadata.title.has_value) {
    info->title = std::make_optional(std::string(metadata.title.value));
  }

  if (metadata.homepage.has_value) {
    info->homepage = std::make_optional(std::string(metadata.homepage.value));
  } else {
    info->homepage = std::nullopt;
  }

  if (metadata.expires_hours.has_value) {
    info->expires = metadata.expires_hours.value;
  } else {
    info->expires = kSubscriptionDefaultExpiresHours;
  }

  UpdateSubscriptionPrefs(sub_url, *info);

  NotifyObserversOfServiceEvent();
}

void AdBlockSubscriptionServiceManager::SetUpdateIntervalsForTesting(
    base::TimeDelta* initial_delay,
    base::TimeDelta* retry_interval) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  g_testing_subscription_retry_interval = retry_interval;
  subscription_update_timer_->Schedule(
      *initial_delay, *retry_interval,
      base::BindRepeating(&AdBlockSubscriptionServiceManager::OnUpdateTimer,
                          weak_ptr_factory_.GetWeakPtr()),
      base::DoNothing());
}

// static
std::optional<SubscriptionInfo> AdBlockSubscriptionServiceManager::GetInfo(
    const GURL& sub_url) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  auto* list_subscription_dict = subscriptions_.FindDict(sub_url.spec());
  if (!list_subscription_dict) {
    return std::nullopt;
  }

  return std::make_optional<SubscriptionInfo>(
      BuildInfoFromDict(sub_url, *list_subscription_dict));
}

void AdBlockSubscriptionServiceManager::LoadSubscriptionServices() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!local_state_) {
    return;
  }

  subscriptions_ =
      local_state_->GetDict(prefs::kAdBlockListSubscriptions).Clone();

  for (const auto it : subscriptions_) {
    const std::string key = it.first;
    SubscriptionInfo info;
    const base::Value::Dict* list_subscription_dict =
        subscriptions_.FindDict(key);
    if (list_subscription_dict) {
      GURL sub_url(key);
      info = BuildInfoFromDict(sub_url, *list_subscription_dict);

      if (info.enabled) {
        auto subscription_filters_provider =
            std::make_unique<AdBlockSubscriptionFiltersProvider>(
                local_state_,
                GetSubscriptionPath(sub_url).Append(
                    kCustomSubscriptionListText),
                base::BindRepeating(
                    &AdBlockSubscriptionServiceManager::OnListMetadata,
                    weak_ptr_factory_.GetWeakPtr(), sub_url));
        subscription_filters_provider->OnListAvailable();
        subscription_filters_providers_.insert(
            std::make_pair(sub_url, std::move(subscription_filters_provider)));
      }
    }
  }
}

// Updates preferences to reflect a new state for the specified filter list
// subscription. Creates the entry if it does not yet exist.
void AdBlockSubscriptionServiceManager::UpdateSubscriptionPrefs(
    const GURL& sub_url,
    const SubscriptionInfo& info) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!local_state_) {
    return;
  }

  {
    ScopedDictPrefUpdate update(local_state_, prefs::kAdBlockListSubscriptions);
    base::Value::Dict& subscriptions = update.Get();
    base::Value::Dict subscription_dict;
    subscription_dict.Set("enabled", info.enabled);
    subscription_dict.Set("last_update_attempt",
                          base::TimeToValue(info.last_update_attempt));
    subscription_dict.Set(
        "last_successful_update_attempt",
        base::TimeToValue(info.last_successful_update_attempt));
    if (info.homepage) {
      subscription_dict.Set("homepage", *info.homepage);
    }
    if (info.title) {
      subscription_dict.Set("title", *info.title);
    }
    subscription_dict.Set("expires", info.expires);
    subscriptions.Set(sub_url.spec(), std::move(subscription_dict));

    // TODO(bridiver) - change to pref registrar
    subscriptions_ = subscriptions.Clone();
  }
  list_p3a_->ReportFilterListUsage();
}

// Updates preferences to remove all state for the specified filter list
// subscription.
void AdBlockSubscriptionServiceManager::ClearSubscriptionPrefs(
    const GURL& sub_url) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!local_state_) {
    return;
  }

  ScopedDictPrefUpdate update(local_state_, prefs::kAdBlockListSubscriptions);
  base::Value::Dict& subscriptions = update.Get();
  subscriptions.Remove(sub_url.spec());

  // TODO(bridiver) - change to pref registrar
  subscriptions_ = subscriptions.Clone();
}

void AdBlockSubscriptionServiceManager::OnSubscriptionDownloaded(
    const GURL& sub_url) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  std::optional<SubscriptionInfo> info = GetInfo(sub_url);

  if (!info) {
    return;
  }

  info->last_update_attempt = base::Time::Now();
  info->last_successful_update_attempt = info->last_update_attempt;
  UpdateSubscriptionPrefs(sub_url, *info);

  auto subscription_filters_provider =
      subscription_filters_providers_.find(sub_url);
  if (subscription_filters_provider != subscription_filters_providers_.end()) {
    subscription_filters_provider->second->OnListAvailable();
  }

  NotifyObserversOfServiceEvent();
}

void AdBlockSubscriptionServiceManager::OnSubscriptionDownloadFailure(
    const GURL& sub_url) {
  std::optional<SubscriptionInfo> info = GetInfo(sub_url);

  if (!info) {
    return;
  }

  info->last_update_attempt = base::Time::Now();
  UpdateSubscriptionPrefs(sub_url, *info);

  NotifyObserversOfServiceEvent();
}

void AdBlockSubscriptionServiceManager::NotifyObserversOfServiceEvent() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  for (auto& observer : observers_) {
    observer.OnServiceUpdateEvent();
  }
}

void AdBlockSubscriptionServiceManager::AddObserver(
    AdBlockSubscriptionServiceManagerObserver* observer) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  observers_.AddObserver(observer);
}

void AdBlockSubscriptionServiceManager::RemoveObserver(
    AdBlockSubscriptionServiceManagerObserver* observer) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  observers_.RemoveObserver(observer);
}

}  // namespace brave_shields
