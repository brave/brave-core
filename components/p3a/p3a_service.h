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
#include "base/gtest_prod_util.h"
#include "base/memory/raw_ref.h"
#include "base/memory/ref_counted.h"
#include "base/metrics/histogram_base.h"
#include "base/metrics/statistics_recorder.h"
#include "brave/components/p3a/message_manager.h"
#include "brave/components/p3a/metric_log_type.h"
#include "brave/components/p3a/p3a_config.h"
#include "brave/components/p3a/remote_config_manager.h"
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
                   public MessageManager::Delegate,
                   public RemoteConfigManager::Delegate {
 public:
  P3AService(PrefService& local_state,
             std::string channel,
             base::Time first_run_time,
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

  // Callbacks are invoked after rotation for a particular log type,
  // before metrics are sent. Useful for just-in-time metrics collection
  base::CallbackListSubscription RegisterRotationCallback(
      base::RepeatingCallback<void(MetricLogType log_type)> callback);
  // Callbacks are invoked for each metric message prepared via Constellation.
  base::CallbackListSubscription RegisterMetricCycledCallback(
      base::RepeatingCallback<void(const std::string& histogram_name)>
          callback);

  bool IsP3AEnabled() const;

  // Needs a living browser process to complete the initialization.
  void Init(scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);

  // Invoked by callbacks registered by our service. Since these callbacks
  // can fire on any thread, this method reposts everything to UI thread.
  void OnHistogramChanged(std::string_view histogram_name,
                          uint64_t name_hash,
                          base::HistogramBase::Sample32 sample);

  // Returns the RemoteConfigManager instance owned by this P3AService
  RemoteConfigManager* remote_config_manager() {
    return remote_config_manager_.get();
  }

  // MessageManager::Delegate
  void OnRotation(MetricLogType log_type) override;
  void OnMetricCycled(const std::string& histogram_name) override;
  std::optional<MetricLogType> GetDynamicMetricLogType(
      std::string_view histogram_name) const override;
  const MetricConfig* GetMetricConfig(
      std::string_view histogram_name) const override;
  std::optional<MetricLogType> GetLogTypeForHistogram(
      std::string_view histogram_name) const override;

  // RemoteConfigManager::Delegate
  void OnRemoteConfigLoaded() override;

 private:
  friend class base::RefCountedThreadSafe<P3AService>;
  friend class P3AServiceTest;
  FRIEND_TEST_ALL_PREFIXES(P3AServiceTest, MessageManagerStartedWhenP3AEnabled);
  FRIEND_TEST_ALL_PREFIXES(P3AServiceTest,
                           MessageManagerNotStartedWhenP3ADisabled);
  FRIEND_TEST_ALL_PREFIXES(P3AServiceTest,
                           MessageManagerStartsAndStopsOnPrefChange);
  FRIEND_TEST_ALL_PREFIXES(P3AServiceTest, MetricValueStored);

  ~P3AService() override;

  void InitCallback(std::string_view histogram_name);

  void LoadDynamicMetrics();

  void OnP3AEnabledChanged();

  // Updates or removes a metric from the log.
  void HandleHistogramChange(std::string_view histogram_name, size_t bucket);

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
  std::unique_ptr<RemoteConfigManager> remote_config_manager_;

  // Used to store histogram values that are produced between constructing
  // the service and its initialization.
  base::flat_map<std::string_view, size_t> histogram_values_;

  std::vector<
      std::unique_ptr<base::StatisticsRecorder::ScopedHistogramSampleObserver>>
      histogram_sample_callbacks_;

  // Contains callbacks registered via `RegisterRotationCallback`
  base::RepeatingCallbackList<void(MetricLogType log_type)> rotation_callbacks_;
  // Contains callbacks registered via `RegisterMetricCycledCallback`
  base::RepeatingCallbackList<void(const std::string& histogram_name)>
      metric_cycled_callbacks_;
};

}  // namespace p3a

#endif  // BRAVE_COMPONENTS_P3A_P3A_SERVICE_H_
