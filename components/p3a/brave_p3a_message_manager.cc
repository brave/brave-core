/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/brave_p3a_message_manager.h"

#include "base/bind.h"
#include "base/i18n/timezone.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_referrals/common/pref_names.h"
#include "brave/components/brave_stats/browser/brave_stats_updater_util.h"
#include "brave/components/p3a/brave_p3a_log_store.h"
#include "brave/components/p3a/brave_p3a_new_uploader.h"
#include "brave/components/p3a/brave_p3a_rotation_scheduler.h"
#include "brave/components/p3a/brave_p3a_scheduler.h"
#include "brave/components/p3a/brave_p3a_star_log_store.h"
#include "brave/components/p3a/brave_p3a_star_manager.h"
#include "brave/components/p3a/pref_names.h"
#include "brave/components/version_info/version_info.h"
#include "brave/vendor/brave_base/random.h"
#include "components/metrics/log_store.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave {

namespace {

const size_t kMaxEpochsToRetain = 4;

base::TimeDelta GetRandomizedUploadInterval(
    base::TimeDelta average_upload_interval) {
  const auto delta = base::Seconds(
      brave_base::random::Geometric(average_upload_interval.InSecondsF()));
  return delta;
}

}  // namespace

BraveP3AMessageManager::BraveP3AMessageManager(PrefService* local_state,
                                               std::string channel,
                                               std::string week_of_install)
    : local_state_(local_state) {
  config_.LoadFromCommandLine();
  InitMessageMeta(channel, week_of_install);
}

BraveP3AMessageManager::~BraveP3AMessageManager() {}

void BraveP3AMessageManager::RegisterPrefs(PrefRegistrySimple* registry) {
  BraveP3ALogStore::RegisterPrefs(registry);
  BraveP3AStarLogStore::RegisterPrefs(registry);
  BraveP3AStarManager::RegisterPrefs(registry);
  BraveP3ARotationScheduler::RegisterPrefs(registry);
}

void BraveP3AMessageManager::Init(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory) {
  // Init log stores.
  json_log_store_.reset(new BraveP3ALogStore(this, local_state_, false));
  json_log_store_->LoadPersistedUnsentLogs();
  star_prep_log_store_.reset(new BraveP3ALogStore(this, local_state_, true));
  star_prep_log_store_->LoadPersistedUnsentLogs();
  star_send_log_store_.reset(
      new BraveP3AStarLogStore(local_state_, kMaxEpochsToRetain));
  star_send_log_store_->LoadPersistedUnsentLogs();

  // Init other components.
  new_uploader_.reset(new BraveP3ANewUploader(
      url_loader_factory,
      base::BindRepeating(&BraveP3AMessageManager::OnLogUploadComplete,
                          base::Unretained(this)),
      GURL(config_.p3a_json_upload_url), GURL(config_.p2a_json_upload_url),
      GURL(config_.p3a_star_upload_url), GURL(config_.p2a_star_upload_url)));

  base::RepeatingCallback<base::TimeDelta(void)> schedule_interval_callback =
      config_.randomize_upload_interval
          ? base::BindRepeating(GetRandomizedUploadInterval,
                                config_.average_upload_interval)
          : base::BindRepeating([](base::TimeDelta x) { return x; },
                                config_.average_upload_interval);

  json_upload_scheduler_.reset(new BraveP3AScheduler(
      base::BindRepeating(&BraveP3AMessageManager::StartScheduledUpload,
                          base::Unretained(this), false),
      schedule_interval_callback));
  star_prep_scheduler_.reset(new BraveP3AScheduler(
      base::BindRepeating(&BraveP3AMessageManager::StartScheduledStarPrep,
                          base::Unretained(this)),
      schedule_interval_callback));
  star_upload_scheduler_.reset(new BraveP3AScheduler(
      base::BindRepeating(&BraveP3AMessageManager::StartScheduledUpload,
                          base::Unretained(this), true),
      schedule_interval_callback));

  json_upload_scheduler_->Start();
  star_upload_scheduler_->Start();

  rotation_scheduler_.reset(new BraveP3ARotationScheduler(
      local_state_, config_.rotation_interval,
      base::BindRepeating(&BraveP3AMessageManager::DoJsonRotation,
                          base::Unretained(this)),
      base::BindRepeating(&BraveP3AMessageManager::DoStarRotation,
                          base::Unretained(this))));

  star_manager_.reset(new BraveP3AStarManager(
      local_state_, url_loader_factory,
      base::BindRepeating(&BraveP3AMessageManager::OnNewStarMessage,
                          base::Unretained(this)),
      base::BindRepeating(&BraveP3AMessageManager::OnRandomnessServerInfoReady,
                          base::Unretained(this)),
      config_.star_randomness_url, config_.star_randomness_info_url,
      config_.use_local_randomness));

  star_manager_->UpdateRandomnessServerInfo();

  VLOG(2) << "BraveP3AMessageManager parameters are:"
          << ", average_upload_interval_ = " << config_.average_upload_interval
          << ", randomize_upload_interval_ = "
          << config_.randomize_upload_interval
          << ", p3a_json_upload_url_ = " << config_.p3a_json_upload_url.spec()
          << ", p2a_json_upload_url_ = " << config_.p2a_json_upload_url.spec()
          << ", p3a_star_upload_url_ = " << config_.p3a_star_upload_url.spec()
          << ", p2a_star_upload_url_ = " << config_.p2a_star_upload_url.spec()
          << ", star_randomness_info_url_ = "
          << config_.star_randomness_info_url.spec()
          << ", star_randomness_url_ = " << config_.star_randomness_url.spec()
          << ", rotation_interval_ = " << config_.rotation_interval;
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
  if (config_.ignore_server_errors) {
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
    const char* histogram_name,
    uint8_t epoch,
    std::unique_ptr<std::string> serialized_message) {
  if (!serialized_message) {
    star_prep_scheduler_->UploadFinished(false);
    return;
  }
  star_prep_scheduler_->UploadFinished(true);
  star_send_log_store_->UpdateMessage(histogram_name, epoch,
                                      *serialized_message);
}

void BraveP3AMessageManager::OnRandomnessServerInfoReady(
    RandomnessServerInfo* server_info) {
  if (server_info == nullptr) {
    return;
  }
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
    star_upload_scheduler_->UploadFinished(true);
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
             "randomness for log";
  if (!star_manager_->StartMessagePreparation(log_key.c_str(), log)) {
    star_upload_scheduler_->UploadFinished(false);
  }
}

void BraveP3AMessageManager::InitMessageMeta(std::string channel,
                                             std::string week_of_install) {
  message_meta_.platform = brave_stats::GetPlatformIdentifier();
  message_meta_.channel = channel;
  message_meta_.version =
      version_info::GetBraveVersionWithoutChromiumMajorVersion();

  if (!week_of_install.empty()) {
    message_meta_.date_of_install = brave_stats::GetYMDAsDate(week_of_install);
  } else {
    message_meta_.date_of_install = base::Time::Now();
  }
  message_meta_.woi =
      brave_stats::GetIsoWeekNumber(message_meta_.date_of_install);

  message_meta_.country_code =
      base::ToUpperASCII(base::CountryCodeForCurrentTimezone());
  message_meta_.refcode = local_state_->GetString(kReferralPromoCode);
  MaybeStripRefcodeAndCountry(&message_meta_);

  UpdateMessageMeta();

  VLOG(2) << "Message meta: " << message_meta_.platform << " "
          << message_meta_.channel << " " << message_meta_.version << " "
          << message_meta_.woi << " " << message_meta_.wos << " "
          << message_meta_.country_code << " " << message_meta_.refcode;
}

void BraveP3AMessageManager::UpdateMessageMeta() {
  message_meta_.date_of_survey = base::Time::Now();
  message_meta_.wos =
      brave_stats::GetIsoWeekNumber(message_meta_.date_of_survey);
}

std::string BraveP3AMessageManager::SerializeLog(
    base::StringPiece histogram_name,
    const uint64_t value,
    bool is_star) {
  UpdateMessageMeta();

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
