/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/brave_p3a_service.h"

#include <memory>
#include <string>

#include "base/command_line.h"
#include "base/i18n/timezone.h"
#include "base/metrics/histogram_samples.h"
#include "base/metrics/metrics_hashes.h"
#include "base/metrics/statistics_recorder.h"
#include "base/rand_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/task/post_task.h"
#include "base/trace_event/trace_event.h"
#include "brave/browser/brave_stats_updater_util.h"
#include "brave/browser/version_info.h"
#include "brave/common/brave_channel_info.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_prochlo/prochlo_message.pb.h"
#include "brave/components/p3a/brave_p3a_log_store.h"
#include "brave/components/p3a/brave_p3a_scheduler.h"
#include "brave/components/p3a/brave_p3a_switches.h"
#include "brave/components/p3a/brave_p3a_uploader.h"
#include "brave/components/p3a/pref_names.h"
#include "brave/vendor/brave_base/random.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_task_traits.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "third_party/metrics_proto/reporting_info.pb.h"

namespace brave {

namespace {

constexpr char kLastRotationTimeStampPref[] = "p3a.last_rotation_timestamp";

constexpr char kDefaultUploadServerUrl[] = "https://p3a.brave.com/";

constexpr uint64_t kDefaultUploadIntervalSeconds = 60;  // 1 minute.

// TODO(iefremov): Provide moar histograms!
// Whitelist for histograms that we collect. Will be replaced with something
// updating on the fly.
constexpr const char* kCollectedHistograms[] = {
    "Brave.P3A.SentAnswersCount",
    "Brave.Savings.BandwidthSavingsMB",
    "Brave.Sync.Status",
    // Deprecated:
    // "DefaultBrowser.State",
    "Brave.Importer.ImporterSource",
    "Brave.Shields.UsageStatus",
    // Do not gather detailed info regarding TOR usage for now.
    // "Brave.Core.LastTimeTorUsed",
    "Brave.Core.IsDefault",
    "Brave.Core.TorEverUsed",
    "Brave.Core.LastTimeIncognitoUsed",
    "Brave.Core.NumberOfExtensions",
    "Brave.Core.BookmarksCountOnProfileLoad",
    "Brave.Core.TabCount",
    "Brave.Core.WindowCount",
    "Brave.Search.DefaultEngine",
    "Brave.Rewards.WalletBalance.2",
    "Brave.Rewards.AutoContributionsState.2",
    "Brave.Rewards.TipsState.2",
    "Brave.Rewards.AdsState.2",
    "Brave.Uptime.BrowserOpenMinutes",
    "Brave.Welcome.InteractionStatus",
};

base::TimeDelta GetRandomizedUploadInterval(
    base::TimeDelta average_upload_interval) {
  return base::TimeDelta::FromSecondsD(
      brave_base::random::Geometric(average_upload_interval.InSecondsF()));
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

  base::TimeDelta result = base::TimeDelta::FromDays(days_till_monday) -
                           (time - time.LocalMidnight());
  return result;
}

}  // namespace

BraveP3AService::BraveP3AService(PrefService* local_state)
    : local_state_(local_state) {}

BraveP3AService::~BraveP3AService() = default;

void BraveP3AService::RegisterPrefs(PrefRegistrySimple* registry,
                                    bool first_run) {
  BraveP3ALogStore::RegisterPrefs(registry);
  registry->RegisterTimePref(kLastRotationTimeStampPref, {});
  registry->RegisterBooleanPref(kP3AEnabled, true);

  // first_run::IsChromeFirstRun() is not available on Android and also
  // we don't have infobars on android.
#if !defined(OS_ANDROID)
  // New users are shown the P3A notice via the welcome page.
  registry->RegisterBooleanPref(kP3ANoticeAcknowledged, first_run);
#endif  // !defined(OS_ANDROID)
}

void BraveP3AService::InitCallbacks() {
  for (const char* histogram_name : kCollectedHistograms) {
    base::StatisticsRecorder::SetCallback(
        histogram_name,
        base::BindRepeating(&BraveP3AService::OnHistogramChanged, this,
                            histogram_name));
  }
}

void BraveP3AService::Init(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory) {
  // Init basic prefs.
  initialized_ = true;

  average_upload_interval_ =
      base::TimeDelta::FromSeconds(kDefaultUploadIntervalSeconds);

  upload_server_url_ = GURL(kDefaultUploadServerUrl);
  MaybeOverrideSettingsFromCommandLine();

  VLOG(2) << "BraveP3AService::Init() Done!";
  VLOG(2) << "BraveP3AService parameters are:"
          << ", average_upload_interval_ = " << average_upload_interval_
          << ", randomize_upload_interval_ = " << randomize_upload_interval_
          << ", upload_server_url_ = " << upload_server_url_.spec()
          << ", rotation_interval_ = " << rotation_interval_;

  InitPyxisMeta();

  // Init log store.
  log_store_.reset(new BraveP3ALogStore(this, local_state_));
  log_store_->LoadPersistedUnsentLogs();
  // Store values that were recorded between calling constructor and |Init()|.
  for (const auto& entry : histogram_values_) {
    log_store_->UpdateValue(entry.first.as_string(), entry.second);
  }
  histogram_values_ = {};
  // Do rotation if needed.
  const base::Time last_rotation =
      local_state_->GetTime(kLastRotationTimeStampPref);
  if (last_rotation.is_null()) {
    DoRotation();
  } else {
    const base::TimeDelta last_rotation_interval =
        rotation_interval_.is_zero() ? TimeDeltaTillMonday(last_rotation)
                                     : rotation_interval_;
    if (base::Time::Now() - last_rotation > last_rotation_interval) {
      DoRotation();
    }
  }

  // Init other components.
  uploader_.reset(new BraveP3AUploader(
      url_loader_factory, upload_server_url_,
      base::Bind(&BraveP3AService::OnLogUploadComplete, this)));

  upload_scheduler_.reset(new BraveP3AScheduler(
      base::Bind(&BraveP3AService::StartScheduledUpload, this),
      (randomize_upload_interval_
           ? base::BindRepeating(GetRandomizedUploadInterval,
                                 average_upload_interval_)
           : base::BindRepeating([](base::TimeDelta x) { return x; },
                                 average_upload_interval_))));

  upload_scheduler_->Start();
  if (!rotation_timer_.IsRunning()) {
    UpdateRotationTimer();
  }
}

std::string BraveP3AService::Serialize(base::StringPiece histogram_name,
                                       uint64_t value) const {
  // TRACE_EVENT0("brave_p3a", "SerializeMessage");
  // TODO(iefremov): Maybe we should store it in logs and pass here?
  // We cannot directly query |base::StatisticsRecorder::FindHistogram| because
  // the serialized value can be obtained from persisted log storage at the
  // point when the actual histogram is not ready yet.
  const uint64_t histogram_name_hash = base::HashMetricName(histogram_name);

  // TODO(iefremov): Restore when PROCHLO/PYXIS is ready.
  //  brave_pyxis::PyxisMessage message;
  //  prochlo::GenerateProchloMessage(histogram_name_hash, value, pyxis_meta_,
  //                                  &message);

  brave_pyxis::RawP3AValue message;
  prochlo::GenerateP3AMessage(histogram_name_hash, value, pyxis_meta_,
                              &message);
  return message.SerializeAsString();
}

bool
BraveP3AService::IsActualMetric(base::StringPiece histogram_name) const {
  static const base::NoDestructor<base::flat_set<base::StringPiece>>
      metric_names {std::begin(kCollectedHistograms),
                    std::end(kCollectedHistograms)};
  return metric_names->contains(histogram_name);
}

void BraveP3AService::MaybeOverrideSettingsFromCommandLine() {
  base::CommandLine* cmdline = base::CommandLine::ForCurrentProcess();

  if (cmdline->HasSwitch(switches::kP3AUploadIntervalSeconds)) {
    std::string seconds_str =
        cmdline->GetSwitchValueASCII(switches::kP3AUploadIntervalSeconds);
    int64_t seconds;
    if (base::StringToInt64(seconds_str, &seconds) && seconds > 0) {
      average_upload_interval_ = base::TimeDelta::FromSeconds(seconds);
    }
  }

  if (cmdline->HasSwitch(switches::kP3ADoNotRandomizeUploadInterval)) {
    randomize_upload_interval_ = false;
  }

  if (cmdline->HasSwitch(switches::kP3ARotationIntervalSeconds)) {
    std::string seconds_str =
        cmdline->GetSwitchValueASCII(switches::kP3ARotationIntervalSeconds);
    int64_t seconds;
    if (base::StringToInt64(seconds_str, &seconds) && seconds > 0) {
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

  // Only upload if service is enabled.
  bool p3a_enabled = local_state_->GetBoolean(brave::kP3AEnabled);
  if (p3a_enabled) {
    const std::string log = log_store_->staged_log();
    VLOG(2) << "StartScheduledUpload - Uploading " << log.size() << " bytes";
    uploader_->UploadLog(log, "", "", {});
  }
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

  base::PostTask(FROM_HERE, {content::BrowserThread::UI},
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
