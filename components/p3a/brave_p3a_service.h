/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_P3A_BRAVE_P3A_SERVICE_H_
#define BRAVE_COMPONENTS_P3A_BRAVE_P3A_SERVICE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/callback_list.h"
#include "base/containers/flat_map.h"
#include "base/memory/ref_counted.h"
#include "base/metrics/histogram_base.h"
#include "base/metrics/statistics_recorder.h"
#include "base/strings/string_piece_forward.h"
#include "brave/components/p3a/brave_p3a_message_manager.h"
#include "brave/components/p3a/metric_log_type.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

class PrefRegistrySimple;
class PrefService;

namespace network {
class SharedURLLoaderFactory;
}

namespace brave {

struct BraveP3AConfig;

// Core class for Brave Privacy-Preserving Product Analytics machinery.
// Works on UI thread. Refcounted to receive histogram updating callbacks
// on any thread.
// TODO(iefremov): It should be possible to get rid of refcounted here.
class BraveP3AService : public base::RefCountedThreadSafe<BraveP3AService>,
                        public BraveP3AMessageManager::Delegate {
 public:
  BraveP3AService(PrefService* local_state,
                  std::string channel,
                  std::string week_of_install);

  BraveP3AService(const BraveP3AService&) = delete;
  BraveP3AService& operator=(const BraveP3AService&) = delete;

  static void RegisterPrefs(PrefRegistrySimple* registry, bool first_run);

  // Should be called right after constructor to subscribe to histogram
  // updates. Can't call it in constructor because of refcounted peculiarities.
  void InitCallbacks();

  // Called by other components to add/remove dynamic metrics
  // (metrics not included in the metric_names.h static list)
  void RegisterDynamicMetric(const std::string& histogram_name,
                             MetricLogType log_type,
                             bool should_be_on_ui_thread = true);
  void RemoveDynamicMetric(const std::string& histogram_name);

  // Callbacks are invoked after rotation for a particular log type,
  // before metrics are sent. Useful for just-in-time metrics collection
  base::CallbackListSubscription RegisterRotationCallback(
      base::RepeatingCallback<void(bool is_express, bool is_star)> callback);
  // Callbacks are invoked for each metric is sent to the P3A JSON server,
  // or STAR message preparation.
  base::CallbackListSubscription RegisterMetricCycledCallback(
      base::RepeatingCallback<void(const std::string& histogram_name,
                                   bool is_star)> callback);

  bool IsP3AEnabled() const;

  // Needs a living browser process to complete the initialization.
  void Init(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);

  // BraveP3AMessageManager::Delegate
  void OnRotation(bool is_express, bool is_star) override;
  void OnMetricCycled(const std::string& histogram_name, bool is_star) override;
  absl::optional<MetricLogType> GetDynamicMetricLogType(
      const std::string& histogram_name) const override;

  // Invoked by callbacks registered by our service. Since these callbacks
  // can fire on any thread, this method reposts everything to UI thread.
  void OnHistogramChanged(const char* histogram_name,
                          uint64_t name_hash,
                          base::HistogramBase::Sample sample);

  void DisableStarAttestationForTesting();

 private:
  friend class base::RefCountedThreadSafe<BraveP3AService>;
  ~BraveP3AService() override;

  void LoadDynamicMetrics();

  void OnHistogramChangedOnUI(const char* histogram_name,
                              base::HistogramBase::Sample sample,
                              size_t bucket);

  // Updates or removes a metric from the log.
  void HandleHistogramChange(base::StringPiece histogram_name, size_t bucket);

  // General prefs:
  bool initialized_ = false;

  PrefService* local_state_;
  std::unique_ptr<BraveP3AConfig> config_;

  // Contains metrics added via `RegisterDynamicMetric`
  base::flat_map<std::string, MetricLogType> dynamic_metric_log_types_;
  base::flat_map<
      std::string,
      std::unique_ptr<base::StatisticsRecorder::ScopedHistogramSampleObserver>>
      dynamic_metric_sample_callbacks_;

  std::unique_ptr<BraveP3AMessageManager> message_manager_;

  // Used to store histogram values that are produced between constructing
  // the service and its initialization.
  base::flat_map<base::StringPiece, size_t> histogram_values_;

  std::vector<
      std::unique_ptr<base::StatisticsRecorder::ScopedHistogramSampleObserver>>
      histogram_sample_callbacks_;

  // Contains callbacks registered via `RegisterRotationCallback`
  base::RepeatingCallbackList<void(bool is_express, bool is_star)>
      rotation_callbacks_;
  // Contains callbacks registered via `RegisterMetricCycledCallback`
  base::RepeatingCallbackList<void(const std::string& histogram_name,
                                   bool is_star)>
      metric_cycled_callbacks_;
};

}  // namespace brave

#endif  // BRAVE_COMPONENTS_P3A_BRAVE_P3A_SERVICE_H_
