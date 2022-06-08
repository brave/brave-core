/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/brave_p3a_message_manager.h"

#include "base/i18n/timezone.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_referrals/common/pref_names.h"
#include "brave/components/brave_stats/browser/brave_stats_updater_util.h"
#include "brave/components/p3a/brave_p3a_log_store.h"
#include "brave/components/p3a/brave_p3a_message_manager_utils.h"
#include "brave/components/p3a/brave_p3a_new_uploader.h"
#include "brave/components/p3a/brave_p3a_scheduler.h"
#include "brave/components/p3a/brave_p3a_star_manager.h"
#include "brave/components/p3a/pref_names.h"
#include "brave/components/version_info/version_info.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave {

namespace {

constexpr char kLastRotationTimeStampPref[] = "p3a.last_rotation_timestamp";

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
  registry->RegisterTimePref(kLastRotationTimeStampPref, {});
}

void BraveP3AMessageManager::Init(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory) {
  // Init log store.
  log_store_.reset(new BraveP3ALogStore(this, local_state_));
  log_store_->LoadPersistedUnsentLogs();

  // Do rotation if needed.
  const base::Time last_rotation =
      local_state_->GetTime(kLastRotationTimeStampPref);
  if (last_rotation.is_null()) {
    DoRotation();
  } else {
    if (!config_.rotation_interval.is_zero()) {
      if (base::Time::Now() - last_rotation > config_.rotation_interval) {
        DoRotation();
      }
    }
    if (base::Time::Now() > NextMonday(last_rotation)) {
      DoRotation();
    }
  }

  // Init other components.
  new_uploader_.reset(new BraveP3ANewUploader(
      url_loader_factory, GURL(config_.p3a_upload_server_url),
      GURL(config_.p2a_upload_server_url)));

  upload_scheduler_.reset(new BraveP3AScheduler(
      base::BindRepeating(&BraveP3AMessageManager::StartScheduledUpload,
                          base::Unretained(this)),
      (config_.randomize_upload_interval
           ? base::BindRepeating(GetRandomizedUploadInterval,
                                 config_.average_upload_interval)
           : base::BindRepeating([](base::TimeDelta x) { return x; },
                                 config_.average_upload_interval))));

  upload_scheduler_->Start();
  if (!rotation_timer_.IsRunning()) {
    UpdateRotationTimer();
  }

  star_manager_.reset(new BraveP3AStarManager(
      url_loader_factory,
      base::BindRepeating(&BraveP3AMessageManager::OnStarMessageCreated,
                          base::Unretained(this)),
      GURL(config_.star_randomness_url), config_.use_local_randomness));

  VLOG(2) << "BraveP3AMessageManager parameters are:"
          << ", average_upload_interval_ = " << config_.average_upload_interval
          << ", randomize_upload_interval_ = "
          << config_.randomize_upload_interval << ", p3a_upload_server_url_ = "
          << config_.p3a_upload_server_url.spec()
          << ", p2a_upload_server_url_ = "
          << config_.p2a_upload_server_url.spec()
          << ", rotation_interval_ = " << config_.rotation_interval;
}

void BraveP3AMessageManager::UpdateMetricValue(base::StringPiece histogram_name,
                                               size_t bucket) {
  log_store_->UpdateValue(std::string(histogram_name), bucket);
}

void BraveP3AMessageManager::RemoveMetricValue(
    base::StringPiece histogram_name) {
  log_store_->RemoveValueIfExists(std::string(histogram_name));
}

void BraveP3AMessageManager::DoRotation() {
  VLOG(2) << "BraveP3AMessageManager doing rotation at " << base::Time::Now();
  log_store_->ResetUploadStamps();
  UpdateRotationTimer();

  local_state_->SetTime(kLastRotationTimeStampPref, base::Time::Now());
}

void BraveP3AMessageManager::UpdateRotationTimer() {
  base::Time now = base::Time::Now();
  base::Time next_rotation = config_.rotation_interval.is_zero()
                                 ? NextMonday(now)
                                 : now + config_.rotation_interval;
  if (now >= next_rotation) {
    // Should never happen, but let's stay on the safe side.
    NOTREACHED();
    return;
  }
  rotation_timer_.Start(FROM_HERE, next_rotation, this,
                        &BraveP3AMessageManager::DoRotation);

  VLOG(2) << "BraveP3AMessageManager new rotation timer will fire at "
          << next_rotation << " after " << next_rotation - now;
}

void BraveP3AMessageManager::OnLogUploadComplete(int response_code,
                                                 int error_code,
                                                 bool was_https) {
  const bool upload_succeeded = response_code == 200;
  bool ok = upload_succeeded;
  if (config_.ignore_server_errors) {
    ok = true;
  }
  VLOG(2) << "BraveP3AMessageManager::UploadFinished ok = " << ok
          << " HTTP response = " << response_code;
  if (ok) {
    log_store_->DiscardStagedLog();
  }
  upload_scheduler_->UploadFinished(ok);
}

void BraveP3AMessageManager::OnStarMessageCreated(
    const char* histogram_name,
    uint8_t epoch,
    std::string serialized_message) {}

void BraveP3AMessageManager::StartScheduledUpload() {
  VLOG(2) << "BraveP3AMessageManager::StartScheduledUpload at "
          << base::Time::Now();
  if (!log_store_->has_unsent_logs()) {
    // We continue to schedule next uploads since new histogram values can
    // come up at any moment. Maybe it's worth to add a method with more
    // appropriate name for this situation.
    upload_scheduler_->UploadFinished(true);
    // Nothing to stage.
    VLOG(2) << "StartScheduledUpload - Nothing to stage.";
    return;
  }
  if (!log_store_->has_staged_log()) {
    log_store_->StageNextLog();
  }

  // Only upload if service is enabled.
  bool p3a_enabled = local_state_->GetBoolean(brave::kP3AEnabled);
  if (p3a_enabled) {
    const std::string log = log_store_->staged_log();
    const std::string log_type = log_store_->staged_log_type();
    VLOG(2) << "StartScheduledUpload - Uploading " << log.size() << " bytes "
            << "of type " << log_type;
    new_uploader_->UploadLog(log, log_type);
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
    const uint64_t value) {
  UpdateMessageMeta();

  base::Value p3a_json_value =
      GenerateP3AMessageDict(histogram_name, value, message_meta_);
  std::string p3a_json_message;
  const bool ok = base::JSONWriter::Write(p3a_json_value, &p3a_json_message);
  DCHECK(ok);

  return p3a_json_message;
}

}  // namespace brave
