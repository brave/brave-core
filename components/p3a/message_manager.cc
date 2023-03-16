/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/message_manager.h"

#include "base/functional/bind.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/p3a/features.h"
#include "brave/components/p3a/metric_names.h"
#include "brave/components/p3a/p3a_config.h"
#include "brave/components/p3a/pref_names.h"
#include "brave/components/p3a/rotation_scheduler.h"
#include "brave/components/p3a/scheduler.h"
#include "brave/components/p3a/star_helper.h"
#include "brave/components/p3a/star_log_store.h"
#include "brave/components/p3a/uploader.h"
#include "components/metrics/log_store.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace p3a {

namespace {

const size_t kMaxEpochsToRetain = 4;
constexpr base::TimeDelta kPostRotationUploadDelay = base::Seconds(30);

bool IsSTAREnabled() {
  return base::FeatureList::IsEnabled(features::kSTAR);
}

}  // namespace

MessageManager::MessageManager(PrefService* local_state,
                               const P3AConfig* config,
                               Delegate* delegate,
                               std::string channel,
                               std::string week_of_install)
    : local_state_(local_state), config_(config), delegate_(delegate) {
  message_meta_.Init(local_state, channel, week_of_install);

  // Init log stores.
  for (MetricLogType log_type : kAllMetricLogTypes) {
    json_log_stores_[log_type] =
        std::make_unique<MetricLogStore>(this, local_state_, false, log_type);
    json_log_stores_[log_type]->LoadPersistedUnsentLogs();
  }
  star_prep_log_store_ = std::make_unique<MetricLogStore>(
      this, local_state_, true, MetricLogType::kTypical);
  star_prep_log_store_->LoadPersistedUnsentLogs();
  star_send_log_store_ =
      std::make_unique<StarLogStore>(local_state_, kMaxEpochsToRetain);
}

MessageManager::~MessageManager() = default;

void MessageManager::RegisterPrefs(PrefRegistrySimple* registry) {
  MetricLogStore::RegisterPrefs(registry);
  StarLogStore::RegisterPrefs(registry);
  StarHelper::RegisterPrefs(registry);
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
  star_prep_scheduler_ = std::make_unique<Scheduler>(
      base::BindRepeating(&MessageManager::StartScheduledStarPrep,
                          base::Unretained(this)),
      config_->randomize_upload_interval, config_->average_upload_interval);
  star_upload_scheduler_ = std::make_unique<Scheduler>(
      base::BindRepeating(&MessageManager::StartScheduledUpload,
                          base::Unretained(this), true,
                          MetricLogType::kTypical),
      config_->randomize_upload_interval, config_->average_upload_interval);

  star_upload_scheduler_->Start();

  rotation_scheduler_ = std::make_unique<RotationScheduler>(
      local_state_, config_.get(),
      base::BindRepeating(&MessageManager::DoJsonRotation,
                          base::Unretained(this)),
      base::BindRepeating(&MessageManager::DoStarRotation,
                          base::Unretained(this)));

  star_helper_ = std::make_unique<StarHelper>(
      local_state_, url_loader_factory,
      base::BindRepeating(&MessageManager::OnNewStarMessage,
                          base::Unretained(this)),
      base::BindRepeating(&MessageManager::OnRandomnessServerInfoReady,
                          base::Unretained(this)),
      config_.get());
  if (IsSTAREnabled()) {
    star_helper_->UpdateRandomnessServerInfo();
  }
}

void MessageManager::UpdateMetricValue(base::StringPiece histogram_name,
                                       size_t bucket) {
  std::string histogram_name_str = std::string(histogram_name);
  MetricLogType log_type = GetLogTypeForHistogram(histogram_name_str);
  if (log_type == MetricLogType::kTypical) {
    // Only update typical metrics, until express/slow STAR metrics are
    // supported
    star_prep_log_store_->UpdateValue(std::string(histogram_name), bucket);
  }
  json_log_stores_[log_type].get()->UpdateValue(std::string(histogram_name),
                                                bucket);
}

void MessageManager::RemoveMetricValue(base::StringPiece histogram_name) {
  for (MetricLogType log_type : kAllMetricLogTypes) {
    json_log_stores_[log_type]->RemoveValueIfExists(
        std::string(histogram_name));
  }
  star_prep_log_store_->RemoveValueIfExists(std::string(histogram_name));
}

void MessageManager::DoJsonRotation(MetricLogType log_type) {
  VLOG(2) << "MessageManager doing json rotation at " << base::Time::Now();
  json_log_stores_[log_type]->ResetUploadStamps();
  delegate_->OnRotation(log_type, false);
}

void MessageManager::DoStarRotation() {
  star_prep_scheduler_->Stop();
  star_prep_log_store_->ResetUploadStamps();
  if (!IsSTAREnabled()) {
    return;
  }
  VLOG(2) << "MessageManager doing star rotation at " << base::Time::Now();
  star_helper_->UpdateRandomnessServerInfo();
  delegate_->OnRotation(MetricLogType::kTypical, true);
}

void MessageManager::OnLogUploadComplete(bool is_ok,
                                         int response_code,
                                         bool is_star,
                                         MetricLogType log_type) {
  VLOG(2) << "MessageManager::UploadFinished ok = " << is_ok
          << " HTTP response = " << response_code;
  if (config_->ignore_server_errors) {
    is_ok = true;
  }
  metrics::LogStore* log_store;
  Scheduler* scheduler;
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

void MessageManager::OnNewStarMessage(
    std::string histogram_name,
    uint8_t epoch,
    std::unique_ptr<std::string> serialized_message) {
  VLOG(2) << "MessageManager::OnNewStarMessage: has val? "
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

void MessageManager::OnRandomnessServerInfoReady(
    RandomnessServerInfo* server_info) {
  if (server_info == nullptr || !IsSTAREnabled()) {
    return;
  }
  VLOG(2) << "MessageManager::OnRandomnessServerInfoReady";
  star_send_log_store_->SetCurrentEpoch(server_info->current_epoch);
  star_send_log_store_->LoadPersistedUnsentLogs();
  star_prep_scheduler_->Start();
  rotation_scheduler_->InitStarTimer(server_info->next_epoch_time);
}

void MessageManager::StartScheduledUpload(bool is_star,
                                          MetricLogType log_type) {
  bool p3a_enabled = local_state_->GetBoolean(p3a::kP3AEnabled);
  if (!p3a_enabled) {
    return;
  }
  metrics::LogStore* log_store;
  Scheduler* scheduler;
  std::string logging_prefix = "MessageManager::StartScheduledUpload (";
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

void MessageManager::StartScheduledStarPrep() {
  bool p3a_enabled = local_state_->GetBoolean(p3a::kP3AEnabled);
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
  VLOG(2) << "MessageManager::StartScheduledStarPrep - starting";
  if (!star_prep_log_store_->has_unsent_logs()) {
    star_prep_scheduler_->UploadFinished(true);
    VLOG(2) << "MessageManager::StartScheduledStarPrep - Nothing to stage.";
    return;
  }
  if (!star_prep_log_store_->has_staged_log()) {
    star_prep_log_store_->StageNextLog();
  }

  const std::string log = star_prep_log_store_->staged_log();
  const std::string log_key = star_prep_log_store_->staged_log_key();
  VLOG(2) << "MessageManager::StartScheduledStarPrep - Requesting "
             "randomness for histogram: "
          << log_key;
  if (!star_helper_->StartMessagePreparation(log_key.c_str(), log)) {
    star_upload_scheduler_->UploadFinished(false);
  }
}

MetricLogType MessageManager::GetLogTypeForHistogram(
    const std::string& histogram_name) {
  MetricLogType result = MetricLogType::kTypical;
  if (p3a::kCollectedExpressHistograms.contains(histogram_name) ||
      delegate_->GetDynamicMetricLogType(histogram_name) ==
          MetricLogType::kExpress) {
    result = MetricLogType::kExpress;
  } else if (p3a::kCollectedSlowHistograms.contains(histogram_name) ||
             delegate_->GetDynamicMetricLogType(histogram_name) ==
                 MetricLogType::kSlow) {
    result = MetricLogType::kSlow;
  }
  return result;
}

std::string MessageManager::SerializeLog(base::StringPiece histogram_name,
                                         const uint64_t value,
                                         MetricLogType log_type,
                                         bool is_star,
                                         const std::string& upload_type) {
  message_meta_.Update();

  if (is_star) {
    return GenerateP3AStarMessage(histogram_name, value, message_meta_);
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
