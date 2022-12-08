/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/brave_p3a_service.h"

#include <string>
#include <utility>

#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "base/metrics/histogram_samples.h"
#include "base/metrics/sample_vector.h"
#include "base/metrics/statistics_recorder.h"
#include "base/no_destructor.h"
#include "base/notreached.h"
#include "base/rand_util.h"
#include "base/strings/string_piece_forward.h"
#include "brave/components/brave_referrals/buildflags/buildflags.h"
#include "brave/components/p3a/brave_p2a_protocols.h"
#include "brave/components/p3a/brave_p3a_message_manager.h"
#include "brave/components/p3a/metric_names.h"
#include "brave/components/p3a/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "third_party/metrics_proto/reporting_info.pb.h"

#if BUILDFLAG(ENABLE_BRAVE_REFERRALS)
#include "brave/components/brave_referrals/common/pref_names.h"
#endif

#if BUILDFLAG(IS_IOS)
#include "ios/web/public/thread/web_task_traits.h"
#include "ios/web/public/thread/web_thread.h"
#else
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#endif  // BUILDFLAG(IS_IOS)

namespace brave {

namespace {

// Receiving this value will effectively prevent the metric from transmission
// to the backend. For now we consider this as a hack for p2a metrics, which
// should be refactored in better times.
constexpr int32_t kSuspendedMetricValue = INT_MAX - 1;
constexpr uint64_t kSuspendedMetricBucket = INT_MAX - 1;

constexpr char kDynamicMetricsDictPref[] = "p3a.dynamic_metrics";

bool IsSuspendedMetric(base::StringPiece metric_name,
                       uint64_t value_or_bucket) {
  return value_or_bucket == kSuspendedMetricBucket;
}

inline scoped_refptr<base::SingleThreadTaskRunner> GetUIThreadTaskRunner() {
#if BUILDFLAG(IS_IOS)
  return web::GetUIThreadTaskRunner({});
#else
  return content::GetUIThreadTaskRunner({});
#endif
}

inline void DCheckCurrentlyOnUIThread() {
#if BUILDFLAG(IS_IOS)
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
#else
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
#endif
}

}  // namespace

BraveP3AService::BraveP3AService(PrefService* local_state,
                                 std::string channel,
                                 std::string week_of_install)
    : local_state_(local_state), config_(new BraveP3AConfig()) {
  config_->LoadFromCommandLine();
  message_manager_ = std::make_unique<BraveP3AMessageManager>(
      local_state, config_.get(), this, channel, week_of_install);
}

BraveP3AService::~BraveP3AService() = default;

void BraveP3AService::RegisterPrefs(PrefRegistrySimple* registry,
                                    bool first_run) {
  BraveP3AMessageManager::RegisterPrefs(registry);
  registry->RegisterBooleanPref(kP3AEnabled, true);

  // New users are shown the P3A notice via the welcome page.
  registry->RegisterBooleanPref(kP3ANoticeAcknowledged, first_run);

  registry->RegisterDictionaryPref(kDynamicMetricsDictPref);
}

void BraveP3AService::InitCallbacks() {
  for (const base::StringPiece& histogram_name :
       p3a::kCollectedTypicalHistograms) {
    histogram_sample_callbacks_.push_back(
        std::make_unique<
            base::StatisticsRecorder::ScopedHistogramSampleObserver>(
            std::string(histogram_name),
            base::BindRepeating(&BraveP3AService::OnHistogramChanged,
                                base::Unretained(this))));
  }
  for (const base::StringPiece& histogram_name :
       p3a::kCollectedExpressHistograms) {
    histogram_sample_callbacks_.push_back(
        std::make_unique<
            base::StatisticsRecorder::ScopedHistogramSampleObserver>(
            std::string(histogram_name),
            base::BindRepeating(&BraveP3AService::OnHistogramChanged,
                                base::Unretained(this))));
  }
  LoadDynamicMetrics();
}

void BraveP3AService::RegisterDynamicMetric(const std::string& histogram_name,
                                            MetricLogType log_type,
                                            bool should_be_on_ui_thread) {
  if (should_be_on_ui_thread) {
    DCheckCurrentlyOnUIThread();
  }
  if (dynamic_metric_log_types_.contains(histogram_name)) {
    return;
  }
  dynamic_metric_log_types_[histogram_name] = log_type;
  dynamic_metric_sample_callbacks_[histogram_name] =
      std::make_unique<base::StatisticsRecorder::ScopedHistogramSampleObserver>(
          std::string(histogram_name),
          base::BindRepeating(&BraveP3AService::OnHistogramChanged, this));

  DictionaryPrefUpdate update(local_state_, kDynamicMetricsDictPref);
  base::Value::Dict& update_dict = update->GetDict();
  update_dict.Set(histogram_name, static_cast<int>(log_type));
}

void BraveP3AService::RemoveDynamicMetric(const std::string& histogram_name) {
  DCheckCurrentlyOnUIThread();
  if (!dynamic_metric_log_types_.contains(histogram_name)) {
    return;
  }
  message_manager_->RemoveMetricValue(histogram_name);
  dynamic_metric_sample_callbacks_.erase(histogram_name);
  dynamic_metric_log_types_.erase(histogram_name);

  DictionaryPrefUpdate update(local_state_, kDynamicMetricsDictPref);
  base::Value::Dict& update_dict = update->GetDict();
  update_dict.Remove(histogram_name);
}

base::CallbackListSubscription BraveP3AService::RegisterRotationCallback(
    base::RepeatingCallback<void(bool is_express, bool is_star)> callback) {
  DCheckCurrentlyOnUIThread();
  return rotation_callbacks_.Add(std::move(callback));
}

base::CallbackListSubscription BraveP3AService::RegisterMetricCycledCallback(
    base::RepeatingCallback<void(const std::string&, bool)> callback) {
  DCheckCurrentlyOnUIThread();
  return metric_cycled_callbacks_.Add(std::move(callback));
}

bool BraveP3AService::IsP3AEnabled() const {
  return local_state_->GetBoolean(brave::kP3AEnabled);
}

void BraveP3AService::Init(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory) {
  message_manager_->Init(url_loader_factory);

  // Init basic prefs.
  initialized_ = true;

  VLOG(2) << "BraveP3AService::Init() Done!";

  // Store values that were recorded between calling constructor and |Init()|.
  for (const auto& entry : histogram_values_) {
    HandleHistogramChange(std::string(entry.first), entry.second);
  }
  histogram_values_ = {};
}

void BraveP3AService::OnRotation(bool is_express, bool is_star) {
  rotation_callbacks_.Notify(is_express, is_star);
}

void BraveP3AService::OnMetricCycled(const std::string& histogram_name,
                                     bool is_star) {
  metric_cycled_callbacks_.Notify(histogram_name, is_star);
}

absl::optional<MetricLogType> BraveP3AService::GetDynamicMetricLogType(
    const std::string& histogram_name) const {
  auto log_type_it = dynamic_metric_log_types_.find(histogram_name);
  return log_type_it != dynamic_metric_log_types_.end()
             ? log_type_it->second
             : absl::optional<MetricLogType>();
}

void BraveP3AService::LoadDynamicMetrics() {
  const auto& dict = local_state_->GetDict(kDynamicMetricsDictPref);

  for (const auto [histogram_name, log_type_ordinal] : dict) {
    DCHECK(log_type_ordinal.is_int());
    const MetricLogType log_type =
        static_cast<MetricLogType>(log_type_ordinal.GetInt());

    RegisterDynamicMetric(histogram_name, log_type, false);
  }
}

void BraveP3AService::OnHistogramChanged(const char* histogram_name,
                                         uint64_t name_hash,
                                         base::HistogramBase::Sample sample) {
  DCHECK(histogram_name != nullptr);

  std::unique_ptr<base::HistogramSamples> samples =
      base::StatisticsRecorder::FindHistogram(histogram_name)->SnapshotDelta();

  // Stop now if there's nothing to do.
  if (samples->Iterator()->Done())
    return;

  // Shortcut for the special values, see |kSuspendedMetricValue|
  // description for details.
  if (IsSuspendedMetric(histogram_name, sample)) {
    GetUIThreadTaskRunner()->PostTask(
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
  if (base::StartsWith(histogram_name, "Brave.P2A",
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

  GetUIThreadTaskRunner()->PostTask(
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
    message_manager_->RemoveMetricValue(std::string(histogram_name));
    return;
  }
  message_manager_->UpdateMetricValue(std::string(histogram_name), bucket);
}

void BraveP3AService::DisableStarAttestationForTesting() {
  config_->disable_star_attestation = true;
}

}  // namespace brave
