/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/brave_p3a_service.h"

#include <memory>
#include <string>
#include <utility>

#include "base/command_line.h"
#include "base/i18n/timezone.h"
#include "base/json/json_writer.h"
#include "base/metrics/histogram_macros.h"
#include "base/metrics/histogram_samples.h"
#include "base/metrics/sample_vector.h"
#include "base/metrics/statistics_recorder.h"
#include "base/no_destructor.h"
#include "base/rand_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_piece_forward.h"
#include "base/trace_event/trace_event.h"
#include "brave/components/brave_referrals/common/pref_names.h"
#include "brave/components/brave_stats/browser/brave_stats_updater_util.h"
#include "brave/components/p3a/brave_p2a_protocols.h"
#include "brave/components/p3a/brave_p3a_log_store.h"
#include "brave/components/p3a/brave_p3a_scheduler.h"
#include "brave/components/p3a/brave_p3a_switches.h"
#include "brave/components/p3a/brave_p3a_uploader.h"
#include "brave/components/p3a/metric_names.h"
#include "brave/components/p3a/pref_names.h"
#include "brave/components/version_info/version_info.h"
#include "brave/vendor/brave_base/random.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_task_traits.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "third_party/metrics_proto/reporting_info.pb.h"

namespace brave {

namespace {

// Receiving this value will effectively prevent the metric from transmission
// to the backend. For now we consider this as a hack for p2a metrics, which
// should be refactored in better times.
constexpr int32_t kSuspendedMetricValue = INT_MAX - 1;
constexpr uint64_t kSuspendedMetricBucket = INT_MAX - 1;

constexpr char kLastRotationTimeStampPref[] = "p3a.last_rotation_timestamp";

constexpr char kP3AServerUrl[] = "https://p3a-json.brave.com/";
constexpr char kP2AServerUrl[] = "https://p2a-json.brave.com/";

constexpr uint64_t kDefaultUploadIntervalSeconds = 60;  // 1 minute.

bool IsSuspendedMetric(base::StringPiece metric_name,
                       uint64_t value_or_bucket) {
  return value_or_bucket == kSuspendedMetricBucket;
}

base::TimeDelta GetRandomizedUploadInterval(
    base::TimeDelta average_upload_interval) {
  const auto delta = base::Seconds(
      brave_base::random::Geometric(average_upload_interval.InSecondsF()));
  return delta;
}

base::Time NextMonday(base::Time time) {
  base::Time::Exploded exploded;
  time.LocalMidnight().LocalExplode(&exploded);
  // 1 stands for Monday, 0 for Sunday
  int days_till_monday = 0;
  if (exploded.day_of_week >= 1) {
    days_till_monday = 8 - exploded.day_of_week;
  } else {
    days_till_monday = 1;
  }

  // Adding few hours of padding to prevent potential problems with DST.
  base::Time result =
      (time.LocalMidnight() + base::Days(days_till_monday) + base::Hours(4))
          .LocalMidnight();
  return result;
}

}  // namespace

BraveP3AService::BraveP3AService(PrefService* local_state,
                                 std::string channel,
                                 std::string week_of_install)
    : local_state_(std::move(local_state)),
      channel_(std::move(channel)),
      week_of_install_(week_of_install) {}

BraveP3AService::~BraveP3AService() = default;

void BraveP3AService::RegisterPrefs(PrefRegistrySimple* registry,
                                    bool first_run) {
  BraveP3ALogStore::RegisterPrefs(registry);
  registry->RegisterTimePref(kLastRotationTimeStampPref, {});
  registry->RegisterBooleanPref(kP3AEnabled, true);

  // New users are shown the P3A notice via the welcome page.
  registry->RegisterBooleanPref(kP3ANoticeAcknowledged, first_run);
}

void BraveP3AService::InitCallbacks() {
  for (base::StringPiece histogram_name : p3a::kCollectedHistograms) {
    histogram_sample_callbacks_.push_back(
        std::make_unique<
            base::StatisticsRecorder::ScopedHistogramSampleObserver>(
            std::string(histogram_name),
            base::BindRepeating(&BraveP3AService::OnHistogramChanged, this)));
  }
}

void BraveP3AService::Init(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory) {
  // Init basic prefs.
  initialized_ = true;

  average_upload_interval_ = base::Seconds(kDefaultUploadIntervalSeconds);

  upload_server_url_ = GURL(kP3AServerUrl);
  MaybeOverrideSettingsFromCommandLine();

  VLOG(2) << "BraveP3AService::Init() Done!";
  VLOG(2) << "BraveP3AService parameters are:"
          << ", average_upload_interval_ = " << average_upload_interval_
          << ", randomize_upload_interval_ = " << randomize_upload_interval_
          << ", upload_server_url_ = " << upload_server_url_.spec()
          << ", rotation_interval_ = " << rotation_interval_;

  InitMessageMeta();

  // Init log store.
  log_store_ = std::make_unique<BraveP3ALogStore>(this, local_state_);
  log_store_->LoadPersistedUnsentLogs();
  // Store values that were recorded between calling constructor and |Init()|.
  for (const auto& entry : histogram_values_) {
    HandleHistogramChange(std::string(entry.first), entry.second);
  }
  histogram_values_ = {};
  // Do rotation if needed.
  const base::Time last_rotation =
      local_state_->GetTime(kLastRotationTimeStampPref);
  if (last_rotation.is_null()) {
    DoRotation();
  } else {
    if (!rotation_interval_.is_zero()) {
      if (base::Time::Now() - last_rotation > rotation_interval_) {
        DoRotation();
      }
    }
    if (base::Time::Now() > NextMonday(last_rotation)) {
      DoRotation();
    }
  }

  // Init other components.
  uploader_ = std::make_unique<BraveP3AUploader>(
      url_loader_factory, GURL(kP3AServerUrl), GURL(kP2AServerUrl),
      base::BindRepeating(&BraveP3AService::OnLogUploadComplete, this));

  upload_scheduler_ = std::make_unique<BraveP3AScheduler>(
      base::BindRepeating(&BraveP3AService::StartScheduledUpload, this),
      (randomize_upload_interval_
           ? base::BindRepeating(GetRandomizedUploadInterval,
                                 average_upload_interval_)
           : base::BindRepeating([](base::TimeDelta x) { return x; },
                                 average_upload_interval_)));

  upload_scheduler_->Start();
  if (!rotation_timer_.IsRunning()) {
    UpdateRotationTimer();
  }
}

std::string BraveP3AService::Serialize(base::StringPiece histogram_name,
                                       uint64_t value) {
  // TRACE_EVENT0("brave_p3a", "SerializeMessage");
  // TODO(iefremov): Maybe we should store it in logs and pass here?
  // We cannot directly query |base::StatisticsRecorder::FindHistogram| because
  // the serialized value can be obtained from persisted log storage at the
  // point when the actual histogram is not ready yet.
  UpdateMessageMeta();
  base::Value::Dict p3a_json_value =
      GenerateP3AMessageDict(histogram_name, value, message_meta_);
  std::string p3a_json_message;
  const bool ok = base::JSONWriter::Write(p3a_json_value, &p3a_json_message);
  DCHECK(ok);

  return p3a_json_message;
}

bool
BraveP3AService::IsActualMetric(base::StringPiece histogram_name) const {
  return p3a::kCollectedHistograms.contains(histogram_name);
}

void BraveP3AService::MaybeOverrideSettingsFromCommandLine() {
  base::CommandLine* cmdline = base::CommandLine::ForCurrentProcess();

  if (cmdline->HasSwitch(switches::kP3AUploadIntervalSeconds)) {
    std::string seconds_str =
        cmdline->GetSwitchValueASCII(switches::kP3AUploadIntervalSeconds);
    int64_t seconds;
    if (base::StringToInt64(seconds_str, &seconds) && seconds > 0) {
      average_upload_interval_ = base::Seconds(seconds);
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
      rotation_interval_ = base::Seconds(seconds);
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

void BraveP3AService::InitMessageMeta() {
  message_meta_.platform = brave_stats::GetPlatformIdentifier();
  message_meta_.channel = channel_;
  message_meta_.version =
      version_info::GetBraveVersionWithoutChromiumMajorVersion();

  if (!week_of_install_.empty()) {
    message_meta_.date_of_install = brave_stats::GetYMDAsDate(week_of_install_);
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

void BraveP3AService::UpdateMessageMeta() {
  message_meta_.date_of_survey = base::Time::Now();
  message_meta_.wos =
      brave_stats::GetIsoWeekNumber(message_meta_.date_of_survey);
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
    const std::string log_type = log_store_->staged_log_type();
    VLOG(2) << "StartScheduledUpload - Uploading " << log.size() << " bytes "
            << "of type " << log_type;
    uploader_->UploadLog(log_store_->staged_log(), log_type);
  }
}

void BraveP3AService::OnHistogramChanged(const char* histogram_name,
                                         uint64_t name_hash,
                                         base::HistogramBase::Sample sample) {
  std::unique_ptr<base::HistogramSamples> samples =
      base::StatisticsRecorder::FindHistogram(histogram_name)->SnapshotDelta();

  // Stop now if there's nothing to do.
  if (samples->Iterator()->Done())
    return;

  // Shortcut for the special values, see |kSuspendedMetricValue|
  // description for details.
  if (IsSuspendedMetric(histogram_name, sample)) {
    content::GetUIThreadTaskRunner({})->PostTask(
        FROM_HERE, base::BindOnce(&BraveP3AService::OnHistogramChangedOnUI,
                                  this, histogram_name, kSuspendedMetricValue,
                                  kSuspendedMetricBucket));
    return;
  }

  // Note that we store only buckets, not actual values.
  size_t bucket = 0u;
  const bool ok = samples->Iterator()->GetBucketIndex(&bucket);
  if (!ok) {
    LOG(ERROR) << "Only linear histograms are supported at the moment!";
    NOTREACHED();
    return;
  }

  // Special handling of P2A histograms.
  if (base::StartsWith(histogram_name, "Brave.P2A.",
                       base::CompareCase::SENSITIVE)) {
    // We need the bucket count to make proper perturbation.
    // All P2A metrics should be implemented as linear histograms.
    base::SampleVector* vector =
        static_cast<base::SampleVector*>(samples.get());
    DCHECK(vector);
    const size_t bucket_count = vector->bucket_ranges()->bucket_count() - 1;
    VLOG(2) << "P2A metric " << histogram_name << " has bucket count "
            << bucket_count;

    // Perturb the bucket.
    bucket = DirectEncodingProtocol::Perturb(bucket_count, bucket);
  }

  content::GetUIThreadTaskRunner({})->PostTask(
      FROM_HERE, base::BindOnce(&BraveP3AService::OnHistogramChangedOnUI, this,
                                histogram_name, sample, bucket));
}

void BraveP3AService::OnHistogramChangedOnUI(const char* histogram_name,
                                             base::HistogramBase::Sample sample,
                                             size_t bucket) {
  VLOG(2) << "BraveP3AService::OnHistogramChanged: histogram_name = "
          << histogram_name << " Sample = " << sample << " bucket = " << bucket;
  if (!initialized_) {
    // Will handle it later when ready.
    histogram_values_[histogram_name] = bucket;
  } else {
    HandleHistogramChange(histogram_name, bucket);
  }
}

void BraveP3AService::HandleHistogramChange(base::StringPiece histogram_name,
                                            size_t bucket) {
  if (IsSuspendedMetric(histogram_name, bucket)) {
    log_store_->RemoveValueIfExists(std::string(histogram_name));
    return;
  }
  log_store_->UpdateValue(std::string(histogram_name), bucket);
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
  base::Time now = base::Time::Now();
  base::Time next_rotation =
      rotation_interval_.is_zero() ? NextMonday(now) : now + rotation_interval_;
  if (now >= next_rotation) {
    // Should never happen, but let's stay on the safe side.
    NOTREACHED();
    return;
  }
  rotation_timer_.Start(FROM_HERE, next_rotation, this,
                        &BraveP3AService::DoRotation);

  VLOG(2) << "BraveP3AService new rotation timer will fire at " << next_rotation
          << " after " << next_rotation - now;
}

}  // namespace brave
