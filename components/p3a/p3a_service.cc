/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/p3a_service.h"

#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "base/check.h"
#include "base/logging.h"
#include "base/metrics/histogram_samples.h"
#include "base/metrics/statistics_recorder.h"
#include "base/trace_event/trace_event.h"
#include "brave/components/p3a/managed/component_installer.h"
#include "brave/components/p3a/message_manager.h"
#include "brave/components/p3a/metric_config_utils.h"
#include "brave/components/p3a/metric_names.h"
#include "brave/components/p3a/p3a_config.h"
#include "brave/components/p3a/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

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
// to the backend. For now we consider this as a hack for p3a metrics, which
// should be refactored in better times.
const uint64_t kSuspendedMetricBucket = INT_MAX - 1;

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
                       base::Time first_run_time,
                       P3AConfig config)
    : local_state_(local_state), config_(std::move(config)) {
  LoadDynamicMetrics();
  if (first_run_time.is_null()) {
    first_run_time = base::Time::Now();
  }

  remote_metric_manager_ =
      std::make_unique<RemoteMetricManager>(&local_state, this);
  remote_config_manager_ =
      std::make_unique<RemoteConfigManager>(this, remote_metric_manager_.get());

  message_manager_ = std::make_unique<MessageManager>(
      local_state, &config_, *this, channel, first_run_time);
}

P3AService::~P3AService() = default;

void P3AService::RegisterPrefs(PrefRegistrySimple* registry, bool first_run) {
  MessageManager::RegisterPrefs(registry);
  registry->RegisterBooleanPref(kP3AEnabled, true);

  // New users are shown the P3A notice via the welcome page.
  registry->RegisterBooleanPref(kP3ANoticeAcknowledged, first_run);

  registry->RegisterDictionaryPref(kDynamicMetricsDictPref);
  registry->RegisterDictionaryPref(kActivationDatesDictPref);
  registry->RegisterDictionaryPref(kRemoteMetricStorageDictPref);
}

void P3AService::InitCallback(std::string_view histogram_name) {
  histogram_sample_callbacks_.push_back(
      std::make_unique<base::StatisticsRecorder::ScopedHistogramSampleObserver>(
          histogram_name, base::BindRepeating(&P3AService::OnHistogramChanged,
                                              base::Unretained(this))));
}

void P3AService::InitCallbacks() {
  for (const auto& [histogram_name, _] : kCollectedTypicalHistograms) {
    InitCallback(histogram_name);
  }
  for (const auto& [histogram_name, _] : kCollectedExpressHistograms) {
    InitCallback(histogram_name);
  }
  for (const auto& [histogram_name, _] : kCollectedSlowHistograms) {
    InitCallback(histogram_name);
  }
  for (const auto& [histogram_name, log_type] : dynamic_metric_log_types_) {
    RegisterDynamicMetric(histogram_name, log_type, false);
  }
}

void P3AService::StartTeardown() {
  dynamic_metric_sample_callbacks_.clear();
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
          histogram_name,
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
    base::RepeatingCallback<void(MetricLogType log_type)> callback) {
  DCheckCurrentlyOnUIThread();
  return rotation_callbacks_.Add(std::move(callback));
}

base::CallbackListSubscription P3AService::RegisterMetricCycledCallback(
    base::RepeatingCallback<void(const std::string& histogram_name)> callback) {
  DCheckCurrentlyOnUIThread();
  return metric_cycled_callbacks_.Add(std::move(callback));
}

bool P3AService::IsP3AEnabled() const {
  return local_state_->GetBoolean(kP3AEnabled);
}

void P3AService::Init(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    component_updater::ComponentUpdateService* cus) {
  if (url_loader_factory) {
    url_loader_factory_ = url_loader_factory;
  }

  if (cus) {
    component_update_service_ = cus;
  }

  if (pref_change_registrar_.IsEmpty()) {
    pref_change_registrar_.Init(&*local_state_);
    auto callback = base::BindRepeating(&P3AService::OnP3AEnabledChanged,
                                        base::Unretained(this));
    pref_change_registrar_.Add(kP3AEnabled, callback);
  }

  if (initialized_ || !url_loader_factory_ ||
      !remote_config_manager_->is_loaded()) {
    return;
  }

  initialized_ = true;

  // Store values that were recorded between calling constructor and |Init()|.
  for (const auto& entry : histogram_values_) {
    UpdateMetricValue(std::string(entry.first), entry.second);
  }
  histogram_values_.clear();

  if (IsP3AEnabled()) {
    message_manager_->Start(url_loader_factory_);
  }
}

void P3AService::OnRotation(MetricLogType log_type) {
  rotation_callbacks_.Notify(log_type);
}

void P3AService::OnMetricCycled(const std::string& histogram_name) {
  metric_cycled_callbacks_.Notify(histogram_name);
}

std::optional<MetricLogType> P3AService::GetDynamicMetricLogType(
    std::string_view histogram_name) const {
  auto log_type_it = dynamic_metric_log_types_.find(histogram_name);
  return log_type_it != dynamic_metric_log_types_.end()
             ? log_type_it->second
             : std::optional<MetricLogType>();
}

const MetricConfig* P3AService::GetMetricConfig(
    std::string_view histogram_name) const {
  // First check if there's a remote config for this metric
  if (remote_config_manager_) {
    const auto* remote_config =
        remote_config_manager_->GetRemoteMetricConfig(histogram_name);
    if (remote_config) {
      return remote_config;
    }
  }

  // Fall back to the base config if no remote config exists
  return GetBaseMetricConfig(histogram_name);
}

void P3AService::OnRemoteConfigLoaded() {
  if (!initialized_) {
    Init(nullptr, nullptr);
  } else {
    message_manager_->RemoveObsoleteLogs();
  }
}

std::optional<MetricLogType> P3AService::GetLogTypeForHistogram(
    std::string_view histogram_name) const {
  if (remote_config_manager_) {
    const auto* remote_config =
        remote_config_manager_->GetRemoteMetricConfig(histogram_name);
    if (remote_config && remote_config->cadence) {
      return *remote_config->cadence;
    }
  }

  auto dynamic_metric_log_type = GetDynamicMetricLogType(histogram_name);
  if (dynamic_metric_log_type) {
    return dynamic_metric_log_type;
  }

  return GetBaseLogTypeForHistogram(histogram_name);
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
  if (initialized_) {
    if (IsP3AEnabled()) {
      message_manager_->Start(url_loader_factory_);
    } else {
      message_manager_->Stop();
    }
  }

  MaybeToggleP3AComponent(component_update_service_, this);
}

void P3AService::OnHistogramChanged(std::string_view histogram_name,
                                    uint64_t name_hash,
                                    base::HistogramBase::Sample32 sample) {
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
        FROM_HERE, base::BindOnce(&P3AService::UpdateMetricValue, this,
                                  histogram_name, kSuspendedMetricBucket));
    return;
  }

  // Note that we store only buckets, not actual values.
  size_t bucket = 0u;
  const bool ok = samples->Iterator()->GetBucketIndex(&bucket);
  if (!ok) {
    LOG(ERROR) << "Only linear histograms are supported at the moment!";
    return;
  }

  GetUIThreadTaskRunner()->PostTask(
      FROM_HERE, base::BindOnce(&P3AService::UpdateMetricValue, this,
                                histogram_name, bucket));
}

void P3AService::UpdateMetricValue(std::string_view histogram_name,
                                   size_t bucket) {
  VLOG(2) << "P3AService::OnHistogramChanged: histogram_name = "
          << histogram_name << " Sample = " << bucket;
  if (!initialized_) {
    // Will handle it later when ready.
    histogram_values_[histogram_name] = bucket;
    return;
  }
  if (IsSuspendedMetric(histogram_name, bucket)) {
    message_manager_->RemoveMetricValue(histogram_name);
    return;
  }
  message_manager_->UpdateMetricValue(histogram_name, bucket);
}

}  // namespace p3a
