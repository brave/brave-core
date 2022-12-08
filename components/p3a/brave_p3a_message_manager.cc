/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/brave_p3a_message_manager.h"

#include "base/bind.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/p3a/brave_p3a_rotation_scheduler.h"
#include "brave/components/p3a/brave_p3a_scheduler.h"
#include "brave/components/p3a/brave_p3a_star.h"
#include "brave/components/p3a/brave_p3a_star_log_store.h"
#include "brave/components/p3a/brave_p3a_uploader.h"
#include "brave/components/p3a/features.h"
#include "brave/components/p3a/metric_names.h"
#include "brave/components/p3a/pref_names.h"
#include "components/metrics/log_store.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave {

namespace {

const size_t kMaxEpochsToRetain = 4;
constexpr base::TimeDelta kPostRotationUploadDelay = base::Seconds(30);

bool IsSTAREnabled() {
  return base::FeatureList::IsEnabled(features::kSTAR);
}

}  // namespace

BraveP3AMessageManager::BraveP3AMessageManager(PrefService* local_state,
                                               BraveP3AConfig* config,
                                               Delegate* delegate,
                                               std::string channel,
                                               std::string week_of_install)
    : local_state_(local_state), config_(config), delegate_(delegate) {
  message_meta_.Init(local_state, channel, week_of_install);

  // Init log stores.
  for (MetricLogType log_type : kAllMetricLogTypes) {
    json_log_stores_[log_type] = std::make_unique<BraveP3AMetricLogStore>(
        this, local_state_, false, log_type);
    json_log_stores_[log_type]->LoadPersistedUnsentLogs();
  }
  star_prep_log_store_ = std::make_unique<BraveP3AMetricLogStore>(
      this, local_state_, true, MetricLogType::kTypical);
  star_prep_log_store_->LoadPersistedUnsentLogs();
  star_send_log_store_ =
      std::make_unique<BraveP3AStarLogStore>(local_state_, kMaxEpochsToRetain);
}

BraveP3AMessageManager::~BraveP3AMessageManager() = default;

void BraveP3AMessageManager::RegisterPrefs(PrefRegistrySimple* registry) {
  BraveP3AMetricLogStore::RegisterPrefs(registry);
  BraveP3AStarLogStore::RegisterPrefs(registry);
  BraveP3AStar::RegisterPrefs(registry);
  BraveP3ARotationScheduler::RegisterPrefs(registry);
}

void BraveP3AMessageManager::Init(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory) {
  // Init other components.
  uploader_ = std::make_unique<BraveP3AUploader>(
      url_loader_factory,
      base::BindRepeating(&BraveP3AMessageManager::OnLogUploadComplete,
                          base::Unretained(this)),
      config_);

  for (MetricLogType log_type : kAllMetricLogTypes) {
    json_upload_schedulers_[log_type] = std::make_unique<BraveP3AScheduler>(
        base::BindRepeating(&BraveP3AMessageManager::StartScheduledUpload,
                            base::Unretained(this), false, log_type),
        config_->randomize_upload_interval, config_->average_upload_interval);
    json_upload_schedulers_[log_type]->Start();
  }
  star_prep_scheduler_ = std::make_unique<BraveP3AScheduler>(
      base::BindRepeating(&BraveP3AMessageManager::StartScheduledStarPrep,
                          base::Unretained(this)),
      config_->randomize_upload_interval, config_->average_upload_interval);
  star_upload_scheduler_ = std::make_unique<BraveP3AScheduler>(
      base::BindRepeating(&BraveP3AMessageManager::StartScheduledUpload,
                          base::Unretained(this), true,
                          MetricLogType::kTypical),
      config_->randomize_upload_interval, config_->average_upload_interval);

  star_upload_scheduler_->Start();

  rotation_scheduler_ = std::make_unique<BraveP3ARotationScheduler>(
      local_state_, config_,
      base::BindRepeating(&BraveP3AMessageManager::DoJsonRotation,
                          base::Unretained(this)),
      base::BindRepeating(&BraveP3AMessageManager::DoStarRotation,
                          base::Unretained(this)));

  star_manager_ = std::make_unique<BraveP3AStar>(
      local_state_, url_loader_factory,
      base::BindRepeating(&BraveP3AMessageManager::OnNewStarMessage,
                          base::Unretained(this)),
      base::BindRepeating(&BraveP3AMessageManager::OnRandomnessServerInfoReady,
                          base::Unretained(this)),
      config_);
  if (IsSTAREnabled()) {
    star_manager_->UpdateRandomnessServerInfo();
  }
}

void BraveP3AMessageManager::UpdateMetricValue(base::StringPiece histogram_name,
                                               size_t bucket) {
  BraveP3AMetricLogStore* json_log_store =
      json_log_stores_[MetricLogType::kTypical].get();
  std::string histogram_name_str = std::string(histogram_name);
  if (p3a::kCollectedExpressHistograms.contains(histogram_name) ||
      delegate_->GetDynamicMetricLogType(histogram_name_str) ==
          MetricLogType::kExpress) {
    json_log_store = json_log_stores_[MetricLogType::kExpress].get();
  } else {
    // Only update typical metrics, until express STAR metrics are supported
    star_prep_log_store_->UpdateValue(std::string(histogram_name), bucket);
  }
  json_log_store->UpdateValue(std::string(histogram_name), bucket);
}

void BraveP3AMessageManager::RemoveMetricValue(
    base::StringPiece histogram_name) {
  for (MetricLogType log_type : kAllMetricLogTypes) {
    json_log_stores_[log_type]->RemoveValueIfExists(
        std::string(histogram_name));
  }
  star_prep_log_store_->RemoveValueIfExists(std::string(histogram_name));
}

void BraveP3AMessageManager::DoJsonRotation(MetricLogType log_type) {
  VLOG(2) << "BraveP3AMessageManager doing json rotation at "
          << base::Time::Now();
  json_log_stores_[log_type]->ResetUploadStamps();
  delegate_->OnRotation(log_type == MetricLogType::kExpress, false);
}

void BraveP3AMessageManager::DoStarRotation() {
  star_prep_scheduler_->Stop();
  star_prep_log_store_->ResetUploadStamps();
  if (!IsSTAREnabled()) {
    return;
  }
  VLOG(2) << "BraveP3AMessageManager doing star rotation at "
          << base::Time::Now();
  star_manager_->UpdateRandomnessServerInfo();
  delegate_->OnRotation(false, true);
}

void BraveP3AMessageManager::OnLogUploadComplete(bool is_ok,
                                                 int response_code,
                                                 bool is_star,
                                                 MetricLogType log_type) {
  VLOG(2) << "BraveP3AMessageManager::UploadFinished ok = " << is_ok
          << " HTTP response = " << response_code;
  if (config_->ignore_server_errors) {
    is_ok = true;
  }
  metrics::LogStore* log_store;
  BraveP3AScheduler* scheduler;
  if (is_star) {
    log_store = (metrics::LogStore*)star_send_log_store_.get();
    scheduler = star_upload_scheduler_.get();
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

void BraveP3AMessageManager::OnNewStarMessage(
    std::string histogram_name,
    uint8_t epoch,
    std::unique_ptr<std::string> serialized_message) {
  VLOG(2) << "BraveP3AMessageManager::OnNewStarMessage: has val? "
          << (serialized_message != nullptr);
  if (!serialized_message) {
    star_prep_scheduler_->UploadFinished(false);
    return;
  }
  star_send_log_store_->UpdateMessage(histogram_name, epoch,
                                      *serialized_message);
  star_prep_log_store_->DiscardStagedLog();
  star_prep_scheduler_->UploadFinished(true);
  delegate_->OnMetricCycled(histogram_name, true);
}

void BraveP3AMessageManager::OnRandomnessServerInfoReady(
    RandomnessServerInfo* server_info) {
  if (server_info == nullptr || !IsSTAREnabled()) {
    return;
  }
  VLOG(2) << "BraveP3AMessageManager::OnRandomnessServerInfoReady";
  star_send_log_store_->SetCurrentEpoch(server_info->current_epoch);
  star_send_log_store_->LoadPersistedUnsentLogs();
  star_prep_scheduler_->Start();
  rotation_scheduler_->InitStarTimer(server_info->next_epoch_time);
}

void BraveP3AMessageManager::StartScheduledUpload(bool is_star,
                                                  MetricLogType log_type) {
  bool p3a_enabled = local_state_->GetBoolean(brave::kP3AEnabled);
  if (!p3a_enabled) {
    return;
  }
  metrics::LogStore* log_store;
  BraveP3AScheduler* scheduler;
  std::string logging_prefix = "BraveP3AMessageManager::StartScheduledUpload (";
  if (is_star) {
    log_store = star_send_log_store_.get();
    scheduler = star_upload_scheduler_.get();
    logging_prefix += "STAR ";
  } else {
    log_store = json_log_stores_[log_type].get();
    scheduler = json_upload_schedulers_[log_type].get();
    logging_prefix += "JSON ";
  }
  logging_prefix += MetricLogTypeToString(log_type);
  logging_prefix += ")";

  if (!is_star &&
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
      is_star ? star_send_log_store_->staged_log_type()
              : json_log_stores_[log_type]->staged_log_type();
  VLOG(2) << logging_prefix << " - Uploading " << log.size() << " bytes";
  uploader_->UploadLog(log, upload_type, is_star, log_type);
}

void BraveP3AMessageManager::StartScheduledStarPrep() {
  bool p3a_enabled = local_state_->GetBoolean(brave::kP3AEnabled);
  if (!p3a_enabled || !IsSTAREnabled()) {
    return;
  }
  if (base::Time::Now() - rotation_scheduler_->GetLastStarRotationTime() <
      kPostRotationUploadDelay) {
    // We should delay STAR preparations right after a rotation to give
    // rotation callbacks a chance to record relevant metrics.
    star_upload_scheduler_->UploadFinished(true);
    return;
  }
  VLOG(2) << "BraveP3AMessageManager::StartScheduledStarPrep - starting";
  if (!star_prep_log_store_->has_unsent_logs()) {
    star_prep_scheduler_->UploadFinished(true);
    VLOG(2)
        << "BraveP3AMessageManager::StartScheduledStarPrep - Nothing to stage.";
    return;
  }
  if (!star_prep_log_store_->has_staged_log()) {
    star_prep_log_store_->StageNextLog();
  }

  const std::string log = star_prep_log_store_->staged_log();
  const std::string log_key = star_prep_log_store_->staged_log_key();
  VLOG(2) << "BraveP3AMessageManager::StartScheduledStarPrep - Requesting "
             "randomness for histogram: "
          << log_key;
  if (!star_manager_->StartMessagePreparation(log_key.c_str(), log)) {
    star_upload_scheduler_->UploadFinished(false);
  }
}

std::string BraveP3AMessageManager::SerializeLog(
    base::StringPiece histogram_name,
    const uint64_t value,
    bool is_star,
    const std::string& upload_type) {
  message_meta_.Update();

  if (is_star) {
    return GenerateP3AStarMessage(histogram_name, value, message_meta_);
  } else {
    base::Value::Dict p3a_json_value = GenerateP3AMessageDict(
        histogram_name, value, message_meta_, upload_type);
    std::string p3a_json_message;
    const bool ok = base::JSONWriter::Write(p3a_json_value, &p3a_json_message);
    DCHECK(ok);

    return p3a_json_message;
  }
}

bool BraveP3AMessageManager::IsActualMetric(
    const std::string& histogram_name) const {
  return p3a::kCollectedTypicalHistograms.contains(histogram_name) ||
         p3a::kCollectedExpressHistograms.contains(histogram_name) ||
         delegate_->GetDynamicMetricLogType(histogram_name).has_value();
}

}  // namespace brave
