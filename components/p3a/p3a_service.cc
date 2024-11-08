/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/p3a_service.h"

#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "base/metrics/histogram_samples.h"
#include "base/metrics/sample_vector.h"
#include "base/metrics/statistics_recorder.h"
#include "base/no_destructor.h"
#include "base/notreached.h"
#include "base/rand_util.h"
#include "base/timer/wall_clock_timer.h"
#include "base/trace_event/trace_event.h"
#include "brave/components/brave_stats/browser/brave_stats_updater_util.h"
#include "brave/components/p3a/message_manager.h"
#include "brave/components/p3a/metric_names.h"
#include "brave/components/p3a/p2a_protocols.h"
#include "brave/components/p3a/p3a_config.h"
#include "brave/components/p3a/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "third_party/metrics_proto/reporting_info.pb.h"

#if BUILDFLAG(IS_IOS)
#include "ios/web/public/thread/web_task_traits.h"
#include "ios/web/public/thread/web_thread.h"
#else
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#endif  // BUILDFLAG(IS_IOS)

namespace p3a {

namespace {

// Receiving this value will effectively prevent the metric from transmission
// to the backend. For now we consider this as a hack for p2a metrics, which
// should be refactored in better times.
const int32_t kSuspendedMetricValue = INT_MAX - 1;
const uint64_t kSuspendedMetricBucket = INT_MAX - 1;

constexpr char kDynamicMetricsDictPref[] = "p3a.dynamic_metrics";

bool IsSuspendedMetric(std::string_view metric_name, uint64_t value_or_bucket) {
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

P3AService::P3AService(PrefService& local_state,
                       std::string channel,
                       std::string week_of_install,
                       P3AConfig config)
    : local_state_(local_state), config_(std::move(config)) {
  LoadDynamicMetrics();
  message_manager_ = std::make_unique<MessageManager>(
      local_state, &config_, *this, channel, week_of_install);
  pref_change_registrar_.Init(&local_state);
  pref_change_registrar_.Add(
      kP3AEnabled, base::BindRepeating(&P3AService::OnP3AEnabledChanged,
                                       base::Unretained(this)));
}

P3AService::~P3AService() = default;

void P3AService::RegisterPrefs(PrefRegistrySimple* registry, bool first_run) {
  MessageManager::RegisterPrefs(registry);
  registry->RegisterBooleanPref(kP3AEnabled, true);

  // New users are shown the P3A notice via the welcome page.
  registry->RegisterBooleanPref(kP3ANoticeAcknowledged, first_run);

  registry->RegisterDictionaryPref(kDynamicMetricsDictPref);
}

void P3AService::InitCallback(std::string_view histogram_name) {
  histogram_sample_callbacks_.push_back(
      std::make_unique<base::StatisticsRecorder::ScopedHistogramSampleObserver>(
          std::string(histogram_name),
          base::BindRepeating(&P3AService::OnHistogramChanged,
                              base::Unretained(this))));
}

void P3AService::InitCallbacks() {
  for (const std::string_view histogram_name :
       p3a::kCollectedTypicalHistograms) {
    InitCallback(histogram_name);
  }
  for (const std::string_view histogram_name :
       p3a::kCollectedExpressHistograms) {
    InitCallback(histogram_name);
  }
  for (const std::string_view histogram_name : p3a::kCollectedSlowHistograms) {
    InitCallback(histogram_name);
  }
  for (const auto& [histogram_name, log_type] : dynamic_metric_log_types_) {
    RegisterDynamicMetric(histogram_name, log_type, false);
  }
}

void P3AService::StartTeardown() {
  pref_change_registrar_.RemoveAll();
}

void P3AService::RegisterDynamicMetric(const std::string& histogram_name,
                                       MetricLogType log_type,
                                       bool should_be_on_ui_thread) {
  if (should_be_on_ui_thread) {
    DCheckCurrentlyOnUIThread();
  }
  if (dynamic_metric_sample_callbacks_.contains(histogram_name)) {
    return;
  }
  dynamic_metric_log_types_[histogram_name] = log_type;
  dynamic_metric_sample_callbacks_[histogram_name] =
      std::make_unique<base::StatisticsRecorder::ScopedHistogramSampleObserver>(
          std::string(histogram_name),
          base::BindRepeating(&P3AService::OnHistogramChanged, this));

  ScopedDictPrefUpdate update(&*local_state_, kDynamicMetricsDictPref);
  update->Set(histogram_name, static_cast<int>(log_type));
}

void P3AService::RemoveDynamicMetric(const std::string& histogram_name) {
  DCheckCurrentlyOnUIThread();
  if (!dynamic_metric_log_types_.contains(histogram_name)) {
    return;
  }
  message_manager_->RemoveMetricValue(histogram_name);
  dynamic_metric_sample_callbacks_.erase(histogram_name);
  dynamic_metric_log_types_.erase(histogram_name);

  ScopedDictPrefUpdate update(&*local_state_, kDynamicMetricsDictPref);
  update->Remove(histogram_name);
}

base::CallbackListSubscription P3AService::RegisterRotationCallback(
    base::RepeatingCallback<void(MetricLogType log_type, bool is_constellation)>
        callback) {
  DCheckCurrentlyOnUIThread();
  return rotation_callbacks_.Add(std::move(callback));
}

base::CallbackListSubscription P3AService::RegisterMetricCycledCallback(
    base::RepeatingCallback<void(const std::string&, bool)> callback) {
  DCheckCurrentlyOnUIThread();
  return metric_cycled_callbacks_.Add(std::move(callback));
}

void P3AService::UpdateMetricValueForSingleFormat(
    const std::string& histogram_name,
    size_t bucket,
    bool is_constellation) {
  DCheckCurrentlyOnUIThread();
  HandleHistogramChange(histogram_name, bucket, is_constellation);
}

bool P3AService::IsP3AEnabled() const {
  return local_state_->GetBoolean(kP3AEnabled);
}

void P3AService::Init(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory) {
  url_loader_factory_ = url_loader_factory;
  if (local_state_->GetBoolean(kP3AEnabled)) {
    message_manager_->Start(url_loader_factory);
  }

  // Init basic prefs.
  initialized_ = true;

  VLOG(2) << "P3AService::Init() Done!";

  // Store values that were recorded between calling constructor and |Init()|.
  for (const auto& entry : histogram_values_) {
    HandleHistogramChange(std::string(entry.first), entry.second);
  }
  histogram_values_ = {};
}

void P3AService::OnRotation(MetricLogType log_type, bool is_constellation) {
  rotation_callbacks_.Notify(log_type, is_constellation);
}

void P3AService::OnMetricCycled(const std::string& histogram_name,
                                bool is_constellation) {
  metric_cycled_callbacks_.Notify(histogram_name, is_constellation);
}

std::optional<MetricLogType> P3AService::GetDynamicMetricLogType(
    const std::string& histogram_name) const {
  auto log_type_it = dynamic_metric_log_types_.find(histogram_name);
  return log_type_it != dynamic_metric_log_types_.end()
             ? log_type_it->second
             : std::optional<MetricLogType>();
}

void P3AService::LoadDynamicMetrics() {
  const auto& dict = local_state_->GetDict(kDynamicMetricsDictPref);

  for (const auto [histogram_name, log_type_ordinal] : dict) {
    DCHECK(log_type_ordinal.is_int());
    const MetricLogType log_type =
        static_cast<MetricLogType>(log_type_ordinal.GetInt());

    dynamic_metric_log_types_[histogram_name] = log_type;
  }
}

void P3AService::OnP3AEnabledChanged() {
  if (local_state_->GetBoolean(kP3AEnabled)) {
    message_manager_->Start(url_loader_factory_);
  } else {
    message_manager_->Stop();
  }
}

void P3AService::OnHistogramChanged(const char* histogram_name,
                                    uint64_t name_hash,
                                    base::HistogramBase::Sample sample) {
  DCHECK(histogram_name != nullptr);

  std::unique_ptr<base::HistogramSamples> samples =
      base::StatisticsRecorder::FindHistogram(histogram_name)->SnapshotDelta();

  // Stop now if there's nothing to do.
  if (samples->Iterator()->Done()) {
    return;
  }

  // Shortcut for the special values, see |kSuspendedMetricValue|
  // description for details.
  if (IsSuspendedMetric(histogram_name, sample)) {
    GetUIThreadTaskRunner()->PostTask(
        FROM_HERE, base::BindOnce(&P3AService::OnHistogramChangedOnUI, this,
                                  histogram_name, kSuspendedMetricValue,
                                  kSuspendedMetricBucket));
    return;
  }

  // Note that we store only buckets, not actual values.
  size_t bucket = 0u;
  const bool ok = samples->Iterator()->GetBucketIndex(&bucket);
  if (!ok) {
    LOG(ERROR) << "Only linear histograms are supported at the moment!";
    return;
  }

  // Special handling of P2A histograms.
  if (std::string_view(histogram_name).starts_with("Brave.P2A")) {
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
      FROM_HERE, base::BindOnce(&P3AService::OnHistogramChangedOnUI, this,
                                histogram_name, sample, bucket));
}

void P3AService::OnHistogramChangedOnUI(const char* histogram_name,
                                        base::HistogramBase::Sample sample,
                                        size_t bucket) {
  VLOG(2) << "P3AService::OnHistogramChanged: histogram_name = "
          << histogram_name << " Sample = " << sample << " bucket = " << bucket;
  if (!initialized_) {
    // Will handle it later when ready.
    histogram_values_[histogram_name] = bucket;
  } else {
    HandleHistogramChange(histogram_name, bucket);
  }
}

void P3AService::HandleHistogramChange(
    std::string_view histogram_name,
    size_t bucket,
    std::optional<bool> only_update_for_constellation) {
  if (IsSuspendedMetric(histogram_name, bucket)) {
    message_manager_->RemoveMetricValue(std::string(histogram_name),
                                        only_update_for_constellation);
    return;
  }
  if (kConstellationOnlyHistograms.contains(histogram_name)) {
    only_update_for_constellation = true;
  }
  message_manager_->UpdateMetricValue(std::string(histogram_name), bucket,
                                      only_update_for_constellation);
}

void P3AService::DisableStarAttestationForTesting() {
  config_.disable_star_attestation = true;
}

}  // namespace p3a
