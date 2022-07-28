/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/brave_p3a_message_manager.h"

#include "base/bind.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/p3a/brave_p3a_new_uploader.h"
#include "brave/components/p3a/brave_p3a_rotation_scheduler.h"
#include "brave/components/p3a/brave_p3a_scheduler.h"
#include "brave/components/p3a/brave_p3a_star.h"
#include "brave/components/p3a/brave_p3a_star_log_store.h"
#include "brave/components/p3a/pref_names.h"
#include "components/metrics/log_store.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave {

namespace {

const size_t kMaxEpochsToRetain = 4;

}  // namespace

BraveP3AMessageManager::BraveP3AMessageManager(PrefService* local_state,
                                               BraveP3AConfig* config,
                                               std::string channel,
                                               std::string week_of_install)
    : local_state_(local_state), config_(config) {
  message_meta_.Init(local_state, channel, week_of_install);

  // Init log stores.
  json_log_store_.reset(new BraveP3AMetricLogStore(this, local_state_, false));
  json_log_store_->LoadPersistedUnsentLogs();
  star_prep_log_store_.reset(
      new BraveP3AMetricLogStore(this, local_state_, true));
  star_prep_log_store_->LoadPersistedUnsentLogs();
  star_send_log_store_.reset(
      new BraveP3AStarLogStore(local_state_, kMaxEpochsToRetain));
}

BraveP3AMessageManager::~BraveP3AMessageManager() {}

void BraveP3AMessageManager::RegisterPrefs(PrefRegistrySimple* registry) {
  BraveP3AMetricLogStore::RegisterPrefs(registry);
  BraveP3AStarLogStore::RegisterPrefs(registry);
  BraveP3AStar::RegisterPrefs(registry);
  BraveP3ARotationScheduler::RegisterPrefs(registry);
}

void BraveP3AMessageManager::Init(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory) {
  // Init other components.
  new_uploader_.reset(new BraveP3ANewUploader(
      url_loader_factory,
      base::BindRepeating(&BraveP3AMessageManager::OnLogUploadComplete,
                          base::Unretained(this)),
      config_));

  json_upload_scheduler_.reset(new BraveP3AScheduler(
      base::BindRepeating(&BraveP3AMessageManager::StartScheduledUpload,
                          base::Unretained(this), false),
      config_->randomize_upload_interval, config_->average_upload_interval));
  star_prep_scheduler_.reset(new BraveP3AScheduler(
      base::BindRepeating(&BraveP3AMessageManager::StartScheduledStarPrep,
                          base::Unretained(this)),
      config_->randomize_upload_interval, config_->average_upload_interval));
  star_upload_scheduler_.reset(new BraveP3AScheduler(
      base::BindRepeating(&BraveP3AMessageManager::StartScheduledUpload,
                          base::Unretained(this), true),
      config_->randomize_upload_interval, config_->average_upload_interval));

  json_upload_scheduler_->Start();
  star_upload_scheduler_->Start();

  rotation_scheduler_.reset(new BraveP3ARotationScheduler(
      local_state_, config_->rotation_interval,
      base::BindRepeating(&BraveP3AMessageManager::DoJsonRotation,
                          base::Unretained(this)),
      base::BindRepeating(&BraveP3AMessageManager::DoStarRotation,
                          base::Unretained(this))));

  star_manager_.reset(new BraveP3AStar(
      local_state_, url_loader_factory,
      base::BindRepeating(&BraveP3AMessageManager::OnNewStarMessage,
                          base::Unretained(this)),
      base::BindRepeating(&BraveP3AMessageManager::OnRandomnessServerInfoReady,
                          base::Unretained(this)),
      config_));
}

void BraveP3AMessageManager::UpdateMetricValue(base::StringPiece histogram_name,
                                               size_t bucket) {
  json_log_store_->UpdateValue(std::string(histogram_name), bucket);
  star_prep_log_store_->UpdateValue(std::string(histogram_name), bucket);
}

void BraveP3AMessageManager::RemoveMetricValue(
    base::StringPiece histogram_name) {
  json_log_store_->RemoveValueIfExists(std::string(histogram_name));
  star_prep_log_store_->RemoveValueIfExists(std::string(histogram_name));
}

void BraveP3AMessageManager::DoJsonRotation() {
  VLOG(2) << "BraveP3AMessageManager doing json rotation at "
          << base::Time::Now();
  json_log_store_->ResetUploadStamps();
}

void BraveP3AMessageManager::DoStarRotation() {
  VLOG(2) << "BraveP3AMessageManager doing star rotation at "
          << base::Time::Now();
  star_prep_scheduler_->Stop();
  star_prep_log_store_->ResetUploadStamps();
  star_manager_->UpdateRandomnessServerInfo();
}

void BraveP3AMessageManager::OnLogUploadComplete(bool is_ok,
                                                 int response_code,
                                                 bool is_star) {
  VLOG(2) << "BraveP3AMessageManager::UploadFinished ok = " << is_ok
          << " HTTP response = " << response_code;
  if (config_->ignore_server_errors) {
    is_ok = true;
  }
  metrics::LogStore* log_store =
      is_star ? (metrics::LogStore*)star_send_log_store_.get()
              : (metrics::LogStore*)json_log_store_.get();
  BraveP3AScheduler* scheduler =
      is_star ? star_upload_scheduler_.get() : json_upload_scheduler_.get();
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
}

void BraveP3AMessageManager::OnRandomnessServerInfoReady(
    RandomnessServerInfo* server_info) {
  if (server_info == nullptr) {
    return;
  }
  VLOG(2) << "BraveP3AMessageManager::OnRandomnessServerInfoReady";
  star_send_log_store_->SetCurrentEpoch(server_info->current_epoch);
  star_send_log_store_->LoadPersistedUnsentLogs();
  star_prep_scheduler_->Start();
  rotation_scheduler_->InitStarTimer(server_info->next_epoch_time);
}

void BraveP3AMessageManager::StartScheduledUpload(bool is_star) {
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
    logging_prefix += "STAR";
  } else {
    log_store = json_log_store_.get();
    scheduler = json_upload_scheduler_.get();
    logging_prefix += "JSON";
  }
  logging_prefix += ")";
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
  const std::string log_type = is_star ? star_send_log_store_->staged_log_type()
                                       : json_log_store_->staged_log_type();
  VLOG(2) << logging_prefix << " - Uploading " << log.size() << " bytes "
          << "of type " << log_type;
  new_uploader_->UploadLog(log, log_type, is_star);
}

void BraveP3AMessageManager::StartScheduledStarPrep() {
  bool p3a_enabled = local_state_->GetBoolean(brave::kP3AEnabled);
  if (!p3a_enabled) {
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
    bool is_star) {
  message_meta_.Update();

  if (is_star) {
    return GenerateP3AStarMessage(histogram_name, value, message_meta_);
  } else {
    base::Value p3a_json_value =
        GenerateP3AMessageDict(histogram_name, value, message_meta_);
    std::string p3a_json_message;
    const bool ok = base::JSONWriter::Write(p3a_json_value, &p3a_json_message);
    DCHECK(ok);

    return p3a_json_message;
  }
}

}  // namespace brave
