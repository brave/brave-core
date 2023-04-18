/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/message_manager.h"

#include "base/functional/bind.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/p3a/constellation_helper.h"
#include "brave/components/p3a/constellation_log_store.h"
#include "brave/components/p3a/features.h"
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

const size_t kMaxEpochsToRetain = 4;
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
    json_log_stores_[log_type] =
        std::make_unique<MetricLogStore>(*this, *local_state_, false, log_type);
    json_log_stores_[log_type]->LoadPersistedUnsentLogs();
  }
  if (features::IsConstellationEnabled()) {
    constellation_prep_log_store_ = std::make_unique<MetricLogStore>(
        *this, *local_state_, true, MetricLogType::kTypical);
    constellation_prep_log_store_->LoadPersistedUnsentLogs();
    constellation_send_log_store_ = std::make_unique<ConstellationLogStore>(
        *local_state_, kMaxEpochsToRetain);
  }
}

MessageManager::~MessageManager() = default;

void MessageManager::RegisterPrefs(PrefRegistrySimple* registry) {
  MetricLogStore::RegisterPrefs(registry);
  ConstellationLogStore::RegisterPrefs(registry);
  ConstellationHelper::RegisterPrefs(registry);
  RotationScheduler::RegisterPrefs(registry);
}

void MessageManager::Init(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory) {
  // Init other components.
  uploader_ = std::make_unique<Uploader>(
      url_loader_factory,
      base::BindRepeating(&MessageManager::OnLogUploadComplete,
                          base::Unretained(this)),
      config_.get());

  for (MetricLogType log_type : kAllMetricLogTypes) {
    json_upload_schedulers_[log_type] = std::make_unique<Scheduler>(
        base::BindRepeating(&MessageManager::StartScheduledUpload,
                            base::Unretained(this), false, log_type),
        config_->randomize_upload_interval, config_->average_upload_interval);
    json_upload_schedulers_[log_type]->Start();
  }
  rotation_scheduler_ = std::make_unique<RotationScheduler>(
      *local_state_, config_.get(),
      base::BindRepeating(&MessageManager::DoJsonRotation,
                          base::Unretained(this)),
      base::BindRepeating(&MessageManager::DoConstellationRotation,
                          base::Unretained(this)));

  if (features::IsConstellationEnabled()) {
    constellation_prep_scheduler_ = std::make_unique<Scheduler>(
        base::BindRepeating(&MessageManager::StartScheduledConstellationPrep,
                            base::Unretained(this)),
        config_->randomize_upload_interval, config_->average_upload_interval);
    constellation_upload_scheduler_ = std::make_unique<Scheduler>(
        base::BindRepeating(&MessageManager::StartScheduledUpload,
                            base::Unretained(this), true,
                            MetricLogType::kTypical),
        config_->randomize_upload_interval, config_->average_upload_interval);

    constellation_upload_scheduler_->Start();

    constellation_helper_ = std::make_unique<ConstellationHelper>(
        &*local_state_, url_loader_factory,
        base::BindRepeating(&MessageManager::OnNewConstellationMessage,
                            base::Unretained(this)),
        base::BindRepeating(&MessageManager::OnRandomnessServerInfoReady,
                            base::Unretained(this)),
        config_.get());
    constellation_helper_->UpdateRandomnessServerInfo();
  }
}

void MessageManager::UpdateMetricValue(base::StringPiece histogram_name,
                                       size_t bucket) {
  MetricLogType log_type = GetLogTypeForHistogram(histogram_name);
  if (features::IsConstellationEnabled()) {
    if (log_type == MetricLogType::kTypical) {
      // Only update typical metrics, until express/slow Constellation metrics
      // are supported
      constellation_prep_log_store_->UpdateValue(std::string(histogram_name),
                                                 bucket);
    }
  }
  json_log_stores_[log_type].get()->UpdateValue(std::string(histogram_name),
                                                bucket);
}

void MessageManager::RemoveMetricValue(base::StringPiece histogram_name) {
  for (MetricLogType log_type : kAllMetricLogTypes) {
    json_log_stores_[log_type]->RemoveValueIfExists(
        std::string(histogram_name));
  }
  if (features::IsConstellationEnabled()) {
    constellation_prep_log_store_->RemoveValueIfExists(
        std::string(histogram_name));
  }
}

void MessageManager::DoJsonRotation(MetricLogType log_type) {
  VLOG(2) << "MessageManager doing json rotation at " << base::Time::Now();
  json_log_stores_[log_type]->ResetUploadStamps();
  delegate_->OnRotation(log_type, false);
}

void MessageManager::DoConstellationRotation() {
  if (!features::IsConstellationEnabled()) {
    return;
  }
  constellation_prep_scheduler_->Stop();
  constellation_prep_log_store_->ResetUploadStamps();
  VLOG(2) << "MessageManager doing Constellation rotation at "
          << base::Time::Now();
  constellation_helper_->UpdateRandomnessServerInfo();
  delegate_->OnRotation(MetricLogType::kTypical, true);
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
    log_store = (metrics::LogStore*)constellation_send_log_store_.get();
    scheduler = constellation_upload_scheduler_.get();
  } else {
    log_store = (metrics::LogStore*)json_log_stores_[log_type].get();
    scheduler = json_upload_schedulers_[log_type].get();
    if (is_ok) {
      delegate_->OnMetricCycled(json_log_stores_[log_type]->staged_log_key(),
                                false);
    }
  }
  if (is_ok) {
    log_store->MarkStagedLogAsSent();
    log_store->DiscardStagedLog();
  }
  scheduler->UploadFinished(is_ok);
}

void MessageManager::OnNewConstellationMessage(
    std::string histogram_name,
    uint8_t epoch,
    std::unique_ptr<std::string> serialized_message) {
  VLOG(2) << "MessageManager::OnNewConstellationMessage: has val? "
          << (serialized_message != nullptr);
  if (!serialized_message) {
    constellation_prep_scheduler_->UploadFinished(false);
    return;
  }
  constellation_send_log_store_->UpdateMessage(histogram_name, epoch,
                                               *serialized_message);
  constellation_prep_log_store_->DiscardStagedLog();
  constellation_prep_scheduler_->UploadFinished(true);
  delegate_->OnMetricCycled(histogram_name, true);
}

void MessageManager::OnRandomnessServerInfoReady(
    RandomnessServerInfo* server_info) {
  if (server_info == nullptr || !features::IsConstellationEnabled()) {
    return;
  }
  VLOG(2) << "MessageManager::OnRandomnessServerInfoReady";
  constellation_send_log_store_->SetCurrentEpoch(server_info->current_epoch);
  constellation_send_log_store_->LoadPersistedUnsentLogs();
  constellation_prep_scheduler_->Start();
  rotation_scheduler_->InitConstellationTimer(server_info->next_epoch_time);
}

void MessageManager::StartScheduledUpload(bool is_constellation,
                                          MetricLogType log_type) {
  bool p3a_enabled = local_state_->GetBoolean(p3a::kP3AEnabled);
  if (!p3a_enabled) {
    return;
  }
  metrics::LogStore* log_store;
  Scheduler* scheduler;
  std::string logging_prefix = "MessageManager::StartScheduledUpload (";
  if (is_constellation) {
    CHECK(features::IsConstellationEnabled());
    log_store = constellation_send_log_store_.get();
    scheduler = constellation_upload_scheduler_.get();
    logging_prefix += "Constellation ";
  } else {
    log_store = json_log_stores_[log_type].get();
    scheduler = json_upload_schedulers_[log_type].get();
    logging_prefix += "JSON ";
  }
  logging_prefix += MetricLogTypeToString(log_type);
  logging_prefix += ")";

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
      is_constellation ? constellation_send_log_store_->staged_log_type()
                       : json_log_stores_[log_type]->staged_log_type();
  VLOG(2) << logging_prefix << " - Uploading " << log.size() << " bytes";
  uploader_->UploadLog(log, upload_type, is_constellation, log_type);
}

void MessageManager::StartScheduledConstellationPrep() {
  CHECK(features::IsConstellationEnabled());
  bool p3a_enabled = local_state_->GetBoolean(p3a::kP3AEnabled);
  if (!p3a_enabled) {
    return;
  }
  if (base::Time::Now() -
          rotation_scheduler_->GetLastConstellationRotationTime() <
      kPostRotationUploadDelay) {
    // We should delay Constellation preparations right after a rotation to give
    // rotation callbacks a chance to record relevant metrics.
    constellation_upload_scheduler_->UploadFinished(true);
    return;
  }
  VLOG(2) << "MessageManager::StartScheduledConstellationPrep - starting";
  if (!constellation_prep_log_store_->has_unsent_logs()) {
    constellation_prep_scheduler_->UploadFinished(true);
    VLOG(2) << "MessageManager::StartScheduledConstellationPrep - Nothing to "
               "stage.";
    return;
  }
  if (!constellation_prep_log_store_->has_staged_log()) {
    constellation_prep_log_store_->StageNextLog();
  }

  const std::string log = constellation_prep_log_store_->staged_log();
  const std::string log_key = constellation_prep_log_store_->staged_log_key();
  VLOG(2) << "MessageManager::StartScheduledConstellationPrep - Requesting "
             "randomness for histogram: "
          << log_key;
  if (!constellation_helper_->StartMessagePreparation(log_key.c_str(), log)) {
    constellation_upload_scheduler_->UploadFinished(false);
  }
}

MetricLogType MessageManager::GetLogTypeForHistogram(
    base::StringPiece histogram_name) {
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

std::string MessageManager::SerializeLog(base::StringPiece histogram_name,
                                         const uint64_t value,
                                         MetricLogType log_type,
                                         bool is_constellation,
                                         const std::string& upload_type) {
  message_meta_.Update();

  if (is_constellation) {
    return GenerateP3AConstellationMessage(histogram_name, value,
                                           message_meta_);
  } else {
    base::Value::Dict p3a_json_value = GenerateP3AMessageDict(
        histogram_name, value, log_type, message_meta_, upload_type);
    std::string p3a_json_message;
    const bool ok = base::JSONWriter::Write(p3a_json_value, &p3a_json_message);
    DCHECK(ok);

    return p3a_json_message;
  }
}

bool MessageManager::IsActualMetric(const std::string& histogram_name) const {
  return p3a::kCollectedTypicalHistograms.contains(histogram_name) ||
         p3a::kCollectedExpressHistograms.contains(histogram_name) ||
         p3a::kCollectedSlowHistograms.contains(histogram_name) ||
         delegate_->GetDynamicMetricLogType(histogram_name).has_value();
}

bool MessageManager::IsEphemeralMetric(
    const std::string& histogram_name) const {
  return p3a::kEphemeralHistograms.contains(histogram_name);
}

}  // namespace p3a
