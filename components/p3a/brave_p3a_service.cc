/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/brave_p3a_service.h"

#include "base/command_line.h"
#include "base/i18n/timezone.h"
#include "base/metrics/histogram_samples.h"
#include "base/metrics/metrics_hashes.h"
#include "base/metrics/statistics_recorder.h"
#include "base/rand_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/task/post_task.h"
#include "brave/browser/brave_stats_updater_util.h"
#include "brave/browser/version_info.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_prochlo/prochlo_message.pb.h"
#include "brave/components/p3a/brave_p3a_log_store.h"
#include "brave/components/p3a/brave_p3a_scheduler.h"
#include "brave/components/p3a/brave_p3a_switches.h"
#include "brave/components/p3a/brave_p3a_uploader.h"
#include "chrome/browser/browser_process.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_task_traits.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "third_party/metrics_proto/reporting_info.pb.h"

namespace brave {

namespace {

constexpr char kLastRotationTimeStampPref[] = "p3a.last_rotation_timestamp";

constexpr char kDefaultUploadServerUrl[] =
    "http://telemetry-poc.bravesoftware.com:8080/";

constexpr uint64_t kDefaultUploadIntervalSeconds = 60 * 60;  // 1 hour.

constexpr int kDefaultMaxRandomDelaySeconds = 5 * 60;  // 5 minutes.

// TODO(iefremov): Provide moar histograms!
// Whitelist for histograms that we collect. Will be replaced with something
// updating on the fly.
constexpr const char* kCollectedHistograms[] = {
    "Brave.P3A.SentAnswersCount",
    "Brave.Sync.Status",
    "DefaultBrowser.State",
    "Extensions.LoadAll",
    "Profile.NumberOfProfiles",
    "Startup.BrowserWindow.FirstPaint"
    "Tabs.TabCount",
};

base::TimeDelta GetRandomizedUploadInterval(base::TimeDelta upload_interval,
                                            uint32_t randomization_delay) {
  if (randomization_delay == 0) {
    return upload_interval;
  }
  return upload_interval + base::TimeDelta::FromSeconds(static_cast<int64_t>(
                               base::RandGenerator(randomization_delay)));
}

base::TimeDelta TimeDeltaTillMonday(base::Time time) {
  base::Time::Exploded exploded;
  time.LocalMidnight().LocalExplode(&exploded);
  // 1 stands for Monday, 0 for Sunday
  int days_till_monday = 0;
  if (exploded.day_of_week >= 1) {
    days_till_monday = 8 - exploded.day_of_week;
  } else {
    days_till_monday = 1;
  }

  base::TimeDelta result =
      base::TimeDelta::FromDays(days_till_monday) - (time - time.LocalMidnight());
  return result;
}

}  // namespace

BraveP3AService::BraveP3AService(PrefService* local_state)
    : local_state_(local_state) {}

BraveP3AService::~BraveP3AService() = default;

void BraveP3AService::RegisterPrefs(PrefRegistrySimple* registry) {
  BraveP3ALogStore::RegisterPrefs(registry);
  registry->RegisterTimePref(kLastRotationTimeStampPref, {});
}

void BraveP3AService::InitCallbacks() {
  for (const char* histogram_name : kCollectedHistograms) {
    base::StatisticsRecorder::SetCallback(
        histogram_name,
        base::BindRepeating(&BraveP3AService::OnHistogramChanged, this,
                            histogram_name));
  }
}

void BraveP3AService::Init() {
  DCHECK(g_browser_process);

  // Init basic prefs.
  initialized_ = true;

  upload_interval_ =
      base::TimeDelta::FromSeconds(kDefaultUploadIntervalSeconds);
  max_random_delay_seconds_ = kDefaultMaxRandomDelaySeconds;

  upload_server_url_ = GURL(kDefaultUploadServerUrl);
  MaybeOverrideSettingsFromCommandLine();

  VLOG(2) << "BraveP3AService::Init() Done!";
  VLOG(2) << "BraveP3AService parameters are:"
          << " upload_enabled_ = " << upload_enabled_
          << ", upload_interval_ = " << upload_interval_
          << ", max_random_delay_seconds_ = " << max_random_delay_seconds_
          << ", upload_server_url_ = " << upload_server_url_.spec()
          << ", rotation_interval_ = " << rotation_interval_;

  InitPyxisMeta();

  // Init log store.
  log_store_.reset(
      new BraveP3ALogStore(this, g_browser_process->local_state()));
  log_store_->LoadPersistedUnsentLogs();
  // Store values that were recorded between calling constructor and |Init()|.
  for (const auto& entry : histogram_values_) {
    log_store_->UpdateValue(entry.first.as_string(), entry.second);
  }
  histogram_values_ = {};
  // Do rotation if needed.
  const base::Time last_rotation =
      local_state_->GetTime(kLastRotationTimeStampPref);
  const base::TimeDelta last_rotation_interval =
      rotation_interval_.is_zero() ? TimeDeltaTillMonday(last_rotation)
                                   : rotation_interval_;
  if (base::Time::Now() - last_rotation > last_rotation_interval) {
    DoRotation();
  }

  // Init other components.
  uploader_.reset(new BraveP3AUploader(
      g_browser_process->shared_url_loader_factory(), upload_server_url_,
      base::Bind(&BraveP3AService::OnLogUploadComplete, this)));

  upload_scheduler_.reset(new BraveP3AScheduler(
      base::Bind(&BraveP3AService::StartScheduledUpload, this),
      base::BindRepeating(GetRandomizedUploadInterval, upload_interval_,
                          max_random_delay_seconds_)));

  // Start the engine if we are enabled.
  if (upload_enabled_) {
    upload_scheduler_->Start();
    if (!rotation_timer_.IsRunning()) {
      UpdateRotationTimer();
    }
  }
}

std::string BraveP3AService::Serialize(base::StringPiece histogram_name,
                                       uint64_t value) const {
  brave_pyxis::PyxisMessage message;
  // TODO(iefremov): Maybe we should store it in logs and pass here?
  // We cannot directly query |base::StatisticsRecorder::FindHistogram| because
  // the serialized value can be obtained from persisted log storage at the
  // point when the actual histogram is not ready yet.
  const uint64_t histogram_name_hash = base::HashMetricName(histogram_name);
  prochlo::GenerateProchloMessage(histogram_name_hash, value, pyxis_meta_,
                                  &message);
  return message.SerializeAsString();
}

void BraveP3AService::MaybeOverrideSettingsFromCommandLine() {
  base::CommandLine* cmdline = base::CommandLine::ForCurrentProcess();

  if (cmdline->HasSwitch(switches::kP3AUploadEnabled)) {
    upload_enabled_ = true;
  }

  if (cmdline->HasSwitch(switches::kP3AUploadIntervalSeconds)) {
    std::string seconds_str =
        cmdline->GetSwitchValueASCII(switches::kP3AUploadIntervalSeconds);
    uint64_t seconds;
    if (base::StringToUint64(seconds_str, &seconds)) {
      upload_interval_ = base::TimeDelta::FromSeconds(seconds);
    }
  }

  if (cmdline->HasSwitch(switches::kP3AUploadMaxRandomDelaySeconds)) {
    std::string seconds_str =
        cmdline->GetSwitchValueASCII(switches::kP3AUploadMaxRandomDelaySeconds);
    base::StringToUint(seconds_str, &max_random_delay_seconds_);
  }

  if (cmdline->HasSwitch(switches::kP3ARotationIntervalSeconds)) {
    std::string seconds_str =
        cmdline->GetSwitchValueASCII(switches::kP3ARotationIntervalSeconds);
    uint64_t seconds;
    if (base::StringToUint64(seconds_str, &seconds)) {
      rotation_interval_ = base::TimeDelta::FromSeconds(seconds);
    }
  }

  if (cmdline->HasSwitch(switches::kP3AUploadServerUrl)) {
    GURL url =
        GURL(cmdline->GetSwitchValueASCII(switches::kP3AUploadServerUrl));
    if (url.is_valid()) {
      upload_server_url_ = url;
    }
  }
}


void BraveP3AService::InitPyxisMeta() {
  pyxis_meta_.platform = brave::GetPlatformIdentifier();
  pyxis_meta_.channel = brave::GetChannelName();
  pyxis_meta_.version =
      version_info::GetBraveVersionWithoutChromiumMajorVersion();

  const std::string woi = local_state_->GetString(kWeekOfInstallation);
  if (!woi.empty()) {
    pyxis_meta_.date_of_install = GetYMDAsDate(woi);
  } else {
    pyxis_meta_.date_of_install = base::Time::Now();
  }
  pyxis_meta_.woi = GetIsoWeekNumber(pyxis_meta_.date_of_install);
  pyxis_meta_.date_of_survey = base::Time::Now();
  pyxis_meta_.wos = GetIsoWeekNumber(pyxis_meta_.date_of_survey);
  pyxis_meta_.country_code =
      base::ToUpperASCII(base::CountryCodeForCurrentTimezone());
  pyxis_meta_.refcode = local_state_->GetString(kReferralPromoCode);
  MaybeStripRefcodeAndCountry(&pyxis_meta_);

  VLOG(2) << "Pyxis meta: " << pyxis_meta_.platform << " "
          << pyxis_meta_.channel << " " << pyxis_meta_.version << " "
          << pyxis_meta_.woi << " " << pyxis_meta_.wos << " "
          << pyxis_meta_.country_code << " " << pyxis_meta_.refcode;
}

void BraveP3AService::StartScheduledUpload() {
  VLOG(2) << "BraveP3AService::StartScheduledUpload at " << base::Time::Now();
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
  const std::string log = log_store_->staged_log();
  VLOG(2) << "StartScheduledUpload - Uploading " << log.size() << " bytes";
  uploader_->UploadLog(log, "", "", {});
}

void BraveP3AService::OnHistogramChanged(base::StringPiece histogram_name,
                                         base::HistogramBase::Sample sample) {

  std::unique_ptr<base::HistogramSamples> samples =
      base::StatisticsRecorder::FindHistogram(histogram_name)->SnapshotDelta();
  DCHECK(!samples->Iterator()->Done());
  // Note that we store only buckets, not actual values.
  size_t bucket = 0u;
  const bool ok = samples->Iterator()->GetBucketIndex(&bucket);
  if (!ok) {
    LOG(ERROR) << "Only linear histograms are supported at the moment!";
    NOTREACHED();
    return;
  }

  base::PostTaskWithTraits(
      FROM_HERE, {content::BrowserThread::UI},
      base::BindOnce(&BraveP3AService::OnHistogramChangedOnUI, this,
                     histogram_name, sample, bucket));
}

void BraveP3AService::OnHistogramChangedOnUI(base::StringPiece histogram_name,
                                             base::HistogramBase::Sample sample,
                                              size_t bucket) {
  VLOG(2) << "BraveP3AService::OnHistogramChanged: histogram_name = "
          << histogram_name << " Sample = " << sample << " bucket = " << bucket;
  if (!initialized_) {
    histogram_values_[histogram_name] = bucket;
  } else {
    log_store_->UpdateValue(histogram_name.as_string(), bucket);
  }
}


void BraveP3AService::OnLogUploadComplete(int response_code,
                                          int error_code,
                                          bool was_https) {
  const bool upload_succeeded = response_code == 200;
  bool ok = upload_succeeded;
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kP3AIgnoreServerErrors)) {
    ok = true;
  }
  VLOG(2) << "BraveP3AService::UploadFinished ok = " << ok
          << " HTTP response = " << response_code;
  if (ok) {
    log_store_->DiscardStagedLog();
  }
  upload_scheduler_->UploadFinished(ok);
}

void BraveP3AService::DoRotation() {
  VLOG(2) << "BraveP3AService doing rotation at " << base::Time::Now();
  log_store_->ResetUploadStamps();
  UpdateRotationTimer();

  local_state_->SetTime(kLastRotationTimeStampPref, base::Time::Now());
}

void BraveP3AService::UpdateRotationTimer() {
  base::TimeDelta next_rotation = rotation_interval_.is_zero()
                                      ? TimeDeltaTillMonday(base::Time::Now())
                                      : rotation_interval_;
  rotation_timer_.Start(FROM_HERE, next_rotation, this,
                        &BraveP3AService::DoRotation);

  VLOG(2) << "BraveP3AService new rotation timer will fire at "
          << base::Time::Now() + next_rotation << " after " << next_rotation;
}
}  // namespace brave
