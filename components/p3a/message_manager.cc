/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/message_manager.h"

#include <optional>
#include <string_view>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/json/values_util.h"
#include "base/logging.h"
#include "base/strings/strcat.h"
#include "brave/components/p3a/constellation_helper.h"
#include "brave/components/p3a/constellation_log_store.h"
#include "brave/components/p3a/features.h"
#include "brave/components/p3a/metric_log_type.h"
#include "brave/components/p3a/metric_names.h"
#include "brave/components/p3a/p3a_config.h"
#include "brave/components/p3a/pref_names.h"
#include "brave/components/p3a/rotation_scheduler.h"
#include "brave/components/p3a/scheduler.h"
#include "brave/components/p3a/uploader.h"
#include "components/metrics/log_store.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace p3a {

namespace {

constexpr base::TimeDelta kPostRotationUploadDelay = base::Seconds(30);

}  // namespace

MessageManager::MessageManager(PrefService& local_state,
                               const P3AConfig* config,
                               Delegate& delegate,
                               std::string channel,
                               base::Time first_run_time)
    : local_state_(local_state), config_(config), delegate_(delegate) {
  message_meta_.Init(&local_state, channel, first_run_time);
  CleanupActivationDates();

  // Init constellation log stores only
  for (MetricLogType log_type : kAllMetricLogTypes) {
    constellation_prep_log_stores_[log_type] =
        std::make_unique<MetricLogStore>(*this, *local_state_, log_type);
    constellation_prep_log_stores_[log_type]->LoadPersistedUnsentLogs();
    constellation_send_log_stores_[log_type] =
        std::make_unique<ConstellationLogStore>(*local_state_, log_type);
  }
}

MessageManager::~MessageManager() = default;

void MessageManager::RegisterPrefs(PrefRegistrySimple* registry) {
  MetricLogStore::RegisterPrefs(registry);
  ConstellationLogStore::RegisterPrefs(registry);
  ConstellationHelper::RegisterPrefs(registry);
  RotationScheduler::RegisterPrefs(registry);
}

void MessageManager::Start(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory) {
  RemoveObsoleteLogs();

  uploader_ = std::make_unique<Uploader>(
      url_loader_factory,
      base::BindRepeating(&MessageManager::OnLogUploadComplete,
                          base::Unretained(this)),
      config_.get());

  constellation_helper_ = std::make_unique<ConstellationHelper>(
      &*local_state_, url_loader_factory,
      base::BindRepeating(&MessageManager::OnNewConstellationMessage,
                          base::Unretained(this)),
      base::BindRepeating(&MessageManager::OnRandomnessServerInfoReady,
                          base::Unretained(this)),
      config_.get());

  rotation_scheduler_ = std::make_unique<RotationScheduler>(
      *local_state_, config_.get(),
      base::BindRepeating(&MessageManager::DoConstellationRotation,
                          base::Unretained(this)));

  for (MetricLogType log_type : kAllMetricLogTypes) {
    constellation_prep_schedulers_[log_type] = std::make_unique<Scheduler>(
        base::BindRepeating(&MessageManager::StartScheduledConstellationPrep,
                            base::Unretained(this), log_type),
        config_->randomize_upload_interval, config_->average_upload_interval);
    constellation_upload_schedulers_[log_type] = std::make_unique<Scheduler>(
        base::BindRepeating(&MessageManager::StartScheduledUpload,
                            base::Unretained(this), log_type),
        config_->randomize_upload_interval, config_->average_upload_interval);

    constellation_upload_schedulers_[log_type]->Start();
    constellation_helper_->UpdateRandomnessServerInfo(log_type);
  }
}

void MessageManager::Stop() {
  uploader_ = nullptr;
  constellation_helper_ = nullptr;
  rotation_scheduler_ = nullptr;
  constellation_prep_schedulers_.clear();
  constellation_upload_schedulers_.clear();
}

void MessageManager::RemoveObsoleteLogs() {
  for (MetricLogType log_type : kAllMetricLogTypes) {
    if (constellation_prep_log_stores_[log_type]) {
      constellation_prep_log_stores_[log_type]->RemoveObsoleteLogs();
    }
  }
}

void MessageManager::UpdateMetricValue(std::string_view histogram_name,
                                       size_t bucket) {
  auto log_type = delegate_->GetLogTypeForHistogram(histogram_name);
  if (!log_type) {
    return;
  }
  constellation_prep_log_stores_[*log_type]->UpdateValue(
      std::string(histogram_name), bucket);
  const auto* metric_config = delegate_->GetMetricConfig(histogram_name);
  if (metric_config && metric_config->record_activation_date && bucket >= 1) {
    // Record activation date for metric, for retention measurement purposes
    ScopedDictPrefUpdate update(&*local_state_, kActivationDatesDictPref);
    if (!update->contains(histogram_name)) {
      update->Set(histogram_name, base::TimeToValue(base::Time::Now()));
    }
  }
}

void MessageManager::RemoveMetricValue(std::string_view histogram_name) {
  for (MetricLogType log_type : kAllMetricLogTypes) {
    constellation_prep_log_stores_[log_type]->RemoveValueIfExists(
        std::string(histogram_name));
  }
}

void MessageManager::DoConstellationRotation(MetricLogType log_type) {
  constellation_prep_schedulers_[log_type]->Stop();
  VLOG(2) << "MessageManager doing Constellation rotation at "
          << base::Time::Now();
  constellation_helper_->UpdateRandomnessServerInfo(log_type);
}

void MessageManager::OnLogUploadComplete(bool is_ok,
                                         int response_code,
                                         MetricLogType log_type) {
  VLOG(2) << "MessageManager::UploadFinished ok = " << is_ok
          << " HTTP response = " << response_code;
  if (config_->ignore_server_errors) {
    is_ok = true;
  }

  auto* log_store = constellation_send_log_stores_[log_type].get();
  Scheduler* scheduler = constellation_upload_schedulers_[log_type].get();

  CHECK(log_store);
  CHECK(scheduler);

  if (is_ok) {
    log_store->MarkStagedLogAsSent();
    log_store->DiscardStagedLog();
  }
  scheduler->UploadFinished(is_ok);
}

void MessageManager::OnNewConstellationMessage(
    std::string histogram_name,
    MetricLogType log_type,
    uint8_t epoch,
    bool is_success,
    std::unique_ptr<std::string> serialized_message) {
  VLOG(2) << "MessageManager::OnNewConstellationMessage: is_success = "
          << is_success << ", has msg = " << (serialized_message != nullptr);
  if (!is_success) {
    constellation_prep_schedulers_[log_type]->UploadFinished(false);
    return;
  }
  // Message may not exist if client did not meet Nebula threshold,
  // check accordingly
  if (serialized_message) {
    constellation_send_log_stores_[log_type]->UpdateMessage(
        histogram_name, epoch, *serialized_message);
  }
  constellation_prep_log_stores_[log_type]->DiscardStagedLog();
  constellation_prep_schedulers_[log_type]->UploadFinished(true);
  delegate_->OnMetricCycled(histogram_name);
}

void MessageManager::OnRandomnessServerInfoReady(
    MetricLogType log_type,
    RandomnessServerInfo* server_info) {
  if (server_info == nullptr) {
    return;
  }
  VLOG(2) << "MessageManager::OnRandomnessServerInfoReady; epoch change = "
          << server_info->epoch_change_detected;
  if (server_info->epoch_change_detected) {
    // a detected epoch change means that we can rotate
    // the preparation store
    constellation_prep_log_stores_[log_type]->ResetUploadStamps();
    delegate_->OnRotation(log_type);
  }
  constellation_send_log_stores_[log_type]->SetCurrentEpoch(
      server_info->current_epoch);
  constellation_send_log_stores_[log_type]->LoadPersistedUnsentLogs();
  constellation_prep_schedulers_[log_type]->Start();
  rotation_scheduler_->InitConstellationTimer(log_type,
                                              server_info->next_epoch_time);
}

void MessageManager::StartScheduledUpload(MetricLogType log_type) {
  CHECK(local_state_->GetBoolean(p3a::kP3AEnabled));

  auto* log_store = constellation_send_log_stores_[log_type].get();
  Scheduler* scheduler = constellation_upload_schedulers_[log_type].get();
  std::string logging_prefix =
      base::StrCat({"MessageManager::StartScheduledUpload ( Constellation, ",
                    MetricLogTypeToString(log_type), ")"});

  CHECK(log_store);
  CHECK(scheduler);

  VLOG(2) << logging_prefix << " at " << base::Time::Now();

  if (!log_store->has_unsent_logs()) {
    scheduler->UploadFinished(true);
    VLOG(2) << logging_prefix << " - Nothing to stage.";
    return;
  }

  if (!log_store->has_staged_log()) {
    log_store->StageNextLog();
  }

  const std::string log = log_store->staged_log();
  const std::string upload_type = log_store->staged_log_type();

  const auto* metric_config =
      delegate_->GetMetricConfig(log_store->staged_log_histogram_name());
  bool is_nebula = metric_config && metric_config->nebula;

  VLOG(2) << logging_prefix << " - Uploading " << log.size() << " bytes";

  uploader_->UploadLog(log, upload_type, is_nebula, log_type);
}

void MessageManager::StartScheduledConstellationPrep(MetricLogType log_type) {
  CHECK(local_state_->GetBoolean(p3a::kP3AEnabled));

  auto* scheduler = constellation_prep_schedulers_[log_type].get();
  auto* log_store = constellation_prep_log_stores_[log_type].get();
  std::string logging_prefix =
      base::StrCat({"MessageManager::StartScheduledConstellationPrep (",
                    MetricLogTypeToString(log_type), ") - "});

  if (base::Time::Now() -
          rotation_scheduler_->GetLastConstellationRotationTime(log_type) <
      kPostRotationUploadDelay) {
    // We should delay Constellation preparations right after a rotation to give
    // rotation callbacks a chance to record relevant metrics.
    scheduler->UploadFinished(true);
    return;
  }

  VLOG(2) << logging_prefix << " - starting";
  if (!log_store->has_unsent_logs()) {
    scheduler->UploadFinished(true);
    VLOG(2) << logging_prefix << " - Nothing to stage.";
    return;
  }
  if (!log_store->has_staged_log()) {
    log_store->StageNextLog();
  }

  const std::string& log = log_store->staged_log();
  const std::string& log_key = log_store->staged_log_key();
  VLOG(2) << logging_prefix
          << " - Requesting randomness for histogram: " << log_key << " "
          << log;

  const auto* metric_config = delegate_->GetMetricConfig(log_key);
  bool is_nebula = metric_config && metric_config->nebula;

  if (!constellation_helper_->StartMessagePreparation(log_key, log_type, log,
                                                      is_nebula)) {
    scheduler->UploadFinished(false);
  }
}

std::optional<MetricLogType> MessageManager::GetLogTypeForHistogram(
    std::string_view histogram_name) const {
  return delegate_->GetLogTypeForHistogram(histogram_name);
}

std::string MessageManager::SerializeLog(std::string_view histogram_name,
                                         const uint64_t value,
                                         MetricLogType log_type,
                                         const std::string& upload_type) {
  message_meta_.Update();

  const auto* metric_config = delegate_->GetMetricConfig(histogram_name);
  return GenerateP3AConstellationMessage(histogram_name, value, message_meta_,
                                         upload_type, metric_config);
}

bool MessageManager::IsEphemeralMetric(
    const std::string& histogram_name) const {
  const auto* metric_config = delegate_->GetMetricConfig(histogram_name);

  return (metric_config && metric_config->ephemeral) ||
         delegate_->GetDynamicMetricLogType(histogram_name).has_value();
}

void MessageManager::CleanupActivationDates() {
  ScopedDictPrefUpdate update(&*local_state_, kActivationDatesDictPref);

  for (auto it = update->begin(); it != update->end();) {
    if (!delegate_->GetLogTypeForHistogram(it->first)) {
      it = update->erase(it);
    } else {
      it++;
    }
  }
}

bool MessageManager::IsActive() const {
  return uploader_ != nullptr;
}

}  // namespace p3a
