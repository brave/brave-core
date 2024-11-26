/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/message_manager.h"

#include <optional>
#include <string_view>

#include "base/functional/bind.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
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
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace p3a {

namespace {

constexpr base::TimeDelta kPostRotationUploadDelay = base::Seconds(30);

}  // namespace

MessageManager::MessageManager(PrefService& local_state,
                               const P3AConfig* config,
                               Delegate& delegate,
                               std::string channel,
                               std::string week_of_install)
    : local_state_(local_state), config_(config), delegate_(delegate) {
  message_meta_.Init(&local_state, channel, week_of_install);

  // Init log stores.
  for (MetricLogType log_type : kAllMetricLogTypes) {
    if (!features::IsJSONDeprecated(log_type)) {
      json_log_stores_[log_type] = std::make_unique<MetricLogStore>(
          *this, *local_state_, false, log_type);
      json_log_stores_[log_type]->LoadPersistedUnsentLogs();
    }
    if (features::IsConstellationEnabled()) {
      constellation_prep_log_stores_[log_type] =
          std::make_unique<MetricLogStore>(*this, *local_state_, true,
                                           log_type);
      constellation_prep_log_stores_[log_type]->LoadPersistedUnsentLogs();
      constellation_send_log_stores_[log_type] =
          std::make_unique<ConstellationLogStore>(*local_state_, log_type);
    }
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
  // Init other components.
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

  for (MetricLogType log_type : kAllMetricLogTypes) {
    if (!features::IsJSONDeprecated(log_type)) {
      json_upload_schedulers_[log_type] = std::make_unique<Scheduler>(
          base::BindRepeating(&MessageManager::StartScheduledUpload,
                              base::Unretained(this), false, log_type),
          config_->randomize_upload_interval, config_->average_upload_interval);
      json_upload_schedulers_[log_type]->Start();
    }
  }

  rotation_scheduler_ = std::make_unique<RotationScheduler>(
      *local_state_, config_.get(),
      base::BindRepeating(&MessageManager::DoJsonRotation,
                          base::Unretained(this)),
      base::BindRepeating(&MessageManager::DoConstellationRotation,
                          base::Unretained(this)));

  for (MetricLogType log_type : kAllMetricLogTypes) {
    if (features::IsConstellationEnabled()) {
      constellation_prep_schedulers_[log_type] = std::make_unique<Scheduler>(
          base::BindRepeating(&MessageManager::StartScheduledConstellationPrep,
                              base::Unretained(this), log_type),
          config_->randomize_upload_interval, config_->average_upload_interval);
      constellation_upload_schedulers_[log_type] = std::make_unique<Scheduler>(
          base::BindRepeating(&MessageManager::StartScheduledUpload,
                              base::Unretained(this), true, log_type),
          config_->randomize_upload_interval, config_->average_upload_interval);

      constellation_upload_schedulers_[log_type]->Start();
      constellation_helper_->UpdateRandomnessServerInfo(log_type);
    }
  }
}

void MessageManager::Stop() {
  uploader_ = nullptr;
  constellation_helper_ = nullptr;
  rotation_scheduler_ = nullptr;
  json_upload_schedulers_.clear();
  constellation_prep_schedulers_.clear();
  constellation_upload_schedulers_.clear();
}

void MessageManager::UpdateMetricValue(
    std::string_view histogram_name,
    size_t bucket,
    std::optional<bool> only_update_for_constellation) {
  bool update_for_all = !only_update_for_constellation.has_value();
  MetricLogType log_type = GetLogTypeForHistogram(histogram_name);
  if (features::IsConstellationEnabled() &&
      (update_for_all || *only_update_for_constellation)) {
    constellation_prep_log_stores_[log_type]->UpdateValue(
        std::string(histogram_name), bucket);
  }
  auto* json_log_store = json_log_stores_[log_type].get();
  if ((update_for_all || !*only_update_for_constellation) && json_log_store) {
    json_log_store->UpdateValue(std::string(histogram_name), bucket);
  }
}

void MessageManager::RemoveMetricValue(
    std::string_view histogram_name,
    std::optional<bool> only_update_for_constellation) {
  bool update_for_all = !only_update_for_constellation.has_value();
  for (MetricLogType log_type : kAllMetricLogTypes) {
    auto* json_log_store = json_log_stores_[log_type].get();
    if ((update_for_all || !*only_update_for_constellation) && json_log_store) {
      json_log_store->RemoveValueIfExists(std::string(histogram_name));
    }
    if (features::IsConstellationEnabled() &&
        (update_for_all || *only_update_for_constellation)) {
      constellation_prep_log_stores_[log_type]->RemoveValueIfExists(
          std::string(histogram_name));
    }
  }
}

void MessageManager::DoJsonRotation(MetricLogType log_type) {
  VLOG(2) << "MessageManager doing json rotation at " << base::Time::Now();
  auto* log_store = json_log_stores_[log_type].get();
  if (log_store) {
    log_store->ResetUploadStamps();
  }
  delegate_->OnRotation(log_type, false);
}

void MessageManager::DoConstellationRotation(MetricLogType log_type) {
  if (!features::IsConstellationEnabled()) {
    return;
  }
  constellation_prep_schedulers_[log_type]->Stop();
  VLOG(2) << "MessageManager doing Constellation rotation at "
          << base::Time::Now();
  constellation_helper_->UpdateRandomnessServerInfo(log_type);
}

void MessageManager::OnLogUploadComplete(bool is_ok,
                                         int response_code,
                                         bool is_constellation,
                                         MetricLogType log_type) {
  VLOG(2) << "MessageManager::UploadFinished ok = " << is_ok
          << " HTTP response = " << response_code;
  if (config_->ignore_server_errors) {
    is_ok = true;
  }
  metrics::LogStore* log_store;
  Scheduler* scheduler;
  if (is_constellation) {
    log_store =
        (metrics::LogStore*)constellation_send_log_stores_[log_type].get();
    scheduler = constellation_upload_schedulers_[log_type].get();
  } else {
    log_store = (metrics::LogStore*)json_log_stores_[log_type].get();
    scheduler = json_upload_schedulers_[log_type].get();
    CHECK(log_store);
    if (is_ok) {
      delegate_->OnMetricCycled(json_log_stores_[log_type]->staged_log_key(),
                                false);
    }
  }
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
  delegate_->OnMetricCycled(histogram_name, true);
}

void MessageManager::OnRandomnessServerInfoReady(
    MetricLogType log_type,
    RandomnessServerInfo* server_info) {
  if (server_info == nullptr || !features::IsConstellationEnabled()) {
    return;
  }
  VLOG(2) << "MessageManager::OnRandomnessServerInfoReady; epoch change = "
          << server_info->epoch_change_detected;
  if (server_info->epoch_change_detected) {
    // a detected epoch change means that we can rotate
    // the preparation store
    constellation_prep_log_stores_[log_type]->ResetUploadStamps();
    delegate_->OnRotation(log_type, true);
  }
  constellation_send_log_stores_[log_type]->SetCurrentEpoch(
      server_info->current_epoch);
  constellation_send_log_stores_[log_type]->LoadPersistedUnsentLogs();
  constellation_prep_schedulers_[log_type]->Start();
  rotation_scheduler_->InitConstellationTimer(log_type,
                                              server_info->next_epoch_time);
}

void MessageManager::StartScheduledUpload(bool is_constellation,
                                          MetricLogType log_type) {
  CHECK(local_state_->GetBoolean(p3a::kP3AEnabled));
  metrics::LogStore* log_store;
  Scheduler* scheduler;
  std::string logging_prefix =
      base::StrCat({"MessageManager::StartScheduledUpload (",
                    is_constellation ? "Constellation" : "JSON", ", ",
                    MetricLogTypeToString(log_type), ")"});

  if (is_constellation) {
    CHECK(features::IsConstellationEnabled());
    log_store = constellation_send_log_stores_[log_type].get();
    scheduler = constellation_upload_schedulers_[log_type].get();
  } else {
    log_store = json_log_stores_[log_type].get();
    scheduler = json_upload_schedulers_[log_type].get();
  }
  CHECK(log_store);
  CHECK(scheduler);

  if (!is_constellation &&
      base::Time::Now() -
              rotation_scheduler_->GetLastJsonRotationTime(log_type) <
          kPostRotationUploadDelay) {
    // We should delay JSON uploads right after a rotation to give
    // rotation callbacks a chance to record relevant metrics.
    scheduler->UploadFinished(true);
    return;
  }

  VLOG(2) << logging_prefix << " at " << base::Time::Now();
  if (!log_store->has_unsent_logs()) {
    // We continue to schedule next uploads since new histogram values can
    // come up at any moment. Maybe it's worth to add a method with more
    // appropriate name for this situation.
    scheduler->UploadFinished(true);
    // Nothing to stage.
    VLOG(2) << logging_prefix << " - Nothing to stage.";
    return;
  }
  if (!log_store->has_staged_log()) {
    log_store->StageNextLog();
  }

  const std::string log = log_store->staged_log();
  const std::string upload_type =
      is_constellation
          ? constellation_send_log_stores_[log_type]->staged_log_type()
          : json_log_stores_[log_type]->staged_log_type();
  bool is_nebula = false;
  if (is_constellation) {
    const auto* metric_config = GetMetricConfig(
        constellation_send_log_stores_[log_type]->staged_log_histogram_name());
    is_nebula = metric_config && *metric_config && (*metric_config)->nebula;
  }

  VLOG(2) << logging_prefix << " - Uploading " << log.size() << " bytes";
  uploader_->UploadLog(log, upload_type, is_constellation, is_nebula, log_type);
}

void MessageManager::StartScheduledConstellationPrep(MetricLogType log_type) {
  CHECK(features::IsConstellationEnabled());
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
  VLOG(2) << "MessageManager::StartScheduledConstellationPrep - starting";
  if (!log_store->has_unsent_logs()) {
    scheduler->UploadFinished(true);
    VLOG(2) << "MessageManager::StartScheduledConstellationPrep - Nothing to "
               "stage.";
    return;
  }
  if (!log_store->has_staged_log()) {
    log_store->StageNextLog();
  }

  const std::string log = log_store->staged_log();
  const std::string log_key = log_store->staged_log_key();
  VLOG(2) << "MessageManager::StartScheduledConstellationPrep - Requesting "
             "randomness for histogram: "
          << log_key << " " << log;

  const auto* metric_config = GetMetricConfig(log_key);
  bool is_nebula = metric_config && *metric_config && (*metric_config)->nebula;
  if (is_nebula && !features::IsNebulaEnabled()) {
    // Do not report if Nebula feature is not enabled,
    // mark request as successful to avoid transmission.
    log_store->DiscardStagedLog();
    scheduler->UploadFinished(true);
    delegate_->OnMetricCycled(log_key, true);
    return;
  }

  if (!constellation_helper_->StartMessagePreparation(log_key.c_str(), log_type,
                                                      log, is_nebula)) {
    scheduler->UploadFinished(false);
  }
}

MetricLogType MessageManager::GetLogTypeForHistogram(
    std::string_view histogram_name) {
  std::string histogram_name_str = std::string(histogram_name);
  MetricLogType result = MetricLogType::kTypical;
  if (p3a::kCollectedExpressHistograms.contains(histogram_name) ||
      delegate_->GetDynamicMetricLogType(histogram_name_str) ==
          MetricLogType::kExpress) {
    result = MetricLogType::kExpress;
  } else if (p3a::kCollectedSlowHistograms.contains(histogram_name) ||
             delegate_->GetDynamicMetricLogType(histogram_name_str) ==
                 MetricLogType::kSlow) {
    result = MetricLogType::kSlow;
  }
  return result;
}

std::string MessageManager::SerializeLog(std::string_view histogram_name,
                                         const uint64_t value,
                                         MetricLogType log_type,
                                         bool is_constellation,
                                         const std::string& upload_type) {
  message_meta_.Update();

  if (is_constellation) {
    const auto* metric_config = GetMetricConfig(histogram_name);
    return GenerateP3AConstellationMessage(
        histogram_name, value, message_meta_, upload_type,
        metric_config ? *metric_config : std::nullopt);
  } else {
    base::Value::Dict p3a_json_value = GenerateP3AMessageDict(
        histogram_name, value, log_type, message_meta_, upload_type);
    std::string p3a_json_message;
    const bool ok = base::JSONWriter::Write(p3a_json_value, &p3a_json_message);
    DCHECK(ok);

    return p3a_json_message;
  }
}

const std::optional<MetricConfig>* MessageManager::GetMetricConfig(
    const std::string_view histogram_name) const {
  const std::optional<MetricConfig>* metric_config = nullptr;

  auto it = p3a::kCollectedTypicalHistograms.find(histogram_name);
  if (it != p3a::kCollectedTypicalHistograms.end()) {
    metric_config = &it->second;
  } else if (it = p3a::kCollectedSlowHistograms.find(histogram_name);
             it != p3a::kCollectedSlowHistograms.end()) {
    metric_config = &it->second;
  } else if (it = p3a::kCollectedExpressHistograms.find(histogram_name);
             it != p3a::kCollectedExpressHistograms.end()) {
    metric_config = &it->second;
  }
  return metric_config;
}

bool MessageManager::IsActualMetric(const std::string& histogram_name) const {
  return p3a::kCollectedTypicalHistograms.contains(histogram_name) ||
         p3a::kCollectedExpressHistograms.contains(histogram_name) ||
         p3a::kCollectedSlowHistograms.contains(histogram_name) ||
         delegate_->GetDynamicMetricLogType(histogram_name).has_value();
}

bool MessageManager::IsEphemeralMetric(
    const std::string& histogram_name) const {
  const auto* metric_config = GetMetricConfig(histogram_name);

  return (metric_config && *metric_config && (*metric_config)->ephemeral) ||
         delegate_->GetDynamicMetricLogType(histogram_name).has_value();
}

}  // namespace p3a
