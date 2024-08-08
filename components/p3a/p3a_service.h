/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_P3A_P3A_SERVICE_H_
#define BRAVE_COMPONENTS_P3A_P3A_SERVICE_H_

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "base/callback_list.h"
#include "base/containers/flat_map.h"
#include "base/memory/raw_ref.h"
#include "base/memory/ref_counted.h"
#include "base/metrics/histogram_base.h"
#include "base/metrics/statistics_recorder.h"
#include "brave/components/p3a/message_manager.h"
#include "brave/components/p3a/metric_log_type.h"
#include "brave/components/p3a/p3a_config.h"
#include "components/prefs/pref_change_registrar.h"

class PrefRegistrySimple;
class PrefService;

namespace network {
class SharedURLLoaderFactory;
}

namespace p3a {

struct P3AConfig;

// Core class for Brave Privacy-Preserving Product Analytics machinery.
// Works on UI thread. Refcounted to receive histogram updating callbacks
// on any thread. This class manages registration of dynamic metrics,
// and histogram listeners. Metric value updates are propagated to the
// P3AMessageManager.
// TODO(iefremov): It should be possible to get rid of refcounted here.
class P3AService : public base::RefCountedThreadSafe<P3AService>,
                   public MessageManager::Delegate {
 public:
  P3AService(PrefService& local_state,
             std::string channel,
             std::string week_of_install,
             P3AConfig config);

  P3AService(const P3AService&) = delete;
  P3AService& operator=(const P3AService&) = delete;

  static void RegisterPrefs(PrefRegistrySimple* registry, bool first_run);

  // Should be called right after constructor to subscribe to histogram
  // updates. Can't call it in constructor because of refcounted peculiarities.
  void InitCallbacks();

  // Should be called in UI thread by BraveBrowserProcess to remove
  // all observers from the PrefChangeRegistrar.
  void StartTeardown();

  // Called by other components to add/remove dynamic metrics
  // (metrics not included in the metric_names.h static list).
  // All dynamic metrics are ephemeral.
  void RegisterDynamicMetric(const std::string& histogram_name,
                             MetricLogType log_type,
                             bool should_be_on_ui_thread = true);
  void RemoveDynamicMetric(const std::string& histogram_name);
  // Updates the metric value for a single upload format (JSON or
  // Constellation). This method is required by NTP/creative metrics; the
  // differing rotation schedules between upload formats require that the
  // differing values are recorded for each format.
  // TODO(djandries): this method should be removed once JSON reporting is fully
  // removed
  void UpdateMetricValueForSingleFormat(const std::string& histogram_name,
                                        size_t bucket,
                                        bool is_constellation);

  // Callbacks are invoked after rotation for a particular log type,
  // before metrics are sent. Useful for just-in-time metrics collection
  base::CallbackListSubscription RegisterRotationCallback(
      base::RepeatingCallback<void(MetricLogType log_type,
                                   bool is_constellation)> callback);
  // Callbacks are invoked for each metric is sent to the P3A JSON server,
  // or Constellation message preparation.
  base::CallbackListSubscription RegisterMetricCycledCallback(
      base::RepeatingCallback<void(const std::string& histogram_name,
                                   bool is_constellation)> callback);

  bool IsP3AEnabled() const;

  // Needs a living browser process to complete the initialization.
  void Init(scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);

  // Invoked by callbacks registered by our service. Since these callbacks
  // can fire on any thread, this method reposts everything to UI thread.
  void OnHistogramChanged(const char* histogram_name,
                          uint64_t name_hash,
                          base::HistogramBase::Sample sample);

  // P3AMessageManager::Delegate
  void OnRotation(MetricLogType log_type, bool is_constellation) override;
  void OnMetricCycled(const std::string& histogram_name,
                      bool is_constellation) override;
  std::optional<MetricLogType> GetDynamicMetricLogType(
      const std::string& histogram_name) const override;

  void DisableStarAttestationForTesting();

 private:
  friend class base::RefCountedThreadSafe<P3AService>;
  ~P3AService() override;

  void InitCallback(const std::string_view histogram_name);

  void LoadDynamicMetrics();

  void OnP3AEnabledChanged();

  void OnHistogramChangedOnUI(const char* histogram_name,
                              base::HistogramBase::Sample sample,
                              size_t bucket);

  // Updates or removes a metric from the log.
  void HandleHistogramChange(
      std::string_view histogram_name,
      size_t bucket,
      std::optional<bool> only_update_for_constellation = std::nullopt);

  // General prefs:
  bool initialized_ = false;

  const raw_ref<PrefService> local_state_;
  P3AConfig config_;

  PrefChangeRegistrar pref_change_registrar_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;

  // Contains metrics added via `RegisterDynamicMetric`
  base::flat_map<std::string, MetricLogType> dynamic_metric_log_types_;
  base::flat_map<
      std::string,
      std::unique_ptr<base::StatisticsRecorder::ScopedHistogramSampleObserver>>
      dynamic_metric_sample_callbacks_;

  std::unique_ptr<MessageManager> message_manager_;

  // Used to store histogram values that are produced between constructing
  // the service and its initialization.
  base::flat_map<std::string_view, size_t> histogram_values_;

  std::vector<
      std::unique_ptr<base::StatisticsRecorder::ScopedHistogramSampleObserver>>
      histogram_sample_callbacks_;

  // Contains callbacks registered via `RegisterRotationCallback`
  base::RepeatingCallbackList<void(MetricLogType log_type,
                                   bool is_constellation)>
      rotation_callbacks_;
  // Contains callbacks registered via `RegisterMetricCycledCallback`
  base::RepeatingCallbackList<void(const std::string& histogram_name,
                                   bool is_constellation)>
      metric_cycled_callbacks_;
};

}  // namespace p3a

#endif  // BRAVE_COMPONENTS_P3A_P3A_SERVICE_H_
