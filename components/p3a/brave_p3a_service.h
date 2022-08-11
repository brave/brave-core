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
#include "base/timer/wall_clock_timer.h"
#include "brave/components/p3a/brave_p3a_log_store.h"
#include "brave/components/p3a/metric_log_type.h"
#include "brave/components/p3a/p3a_message.h"
#include "url/gurl.h"

class PrefRegistrySimple;

namespace network {
class SharedURLLoaderFactory;
}

namespace brave {

class BraveP3AScheduler;
class BraveP3AUploader;

// Core class for Brave Privacy-Preserving Product Analytics machinery.
// Works on UI thread. Refcounted to receive histogram updating callbacks
// on any thread.
// TODO(iefremov): It should be possible to get rid of refcounted here.
class BraveP3AService : public base::RefCountedThreadSafe<BraveP3AService>,
                        public BraveP3ALogStore::Delegate {
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
  bool IsDynamicMetricRegistered(const std::string& histogram_name);

  // Callbacks are invoked after rotation for a particular log type,
  // before metrics are sent. Useful for just-in-time metrics collection
  base::CallbackListSubscription RegisterRotationCallback(
      base::RepeatingCallback<void(bool is_express)> callback);
  // Callbacks are invoked for each metric uploaded to the P3A server.
  base::CallbackListSubscription RegisterMetricSentCallback(
      base::RepeatingCallback<void(const std::string& histogram_name)>
          callback);

  bool IsP3AEnabled();

  // Needs a living browser process to complete the initialization.
  void Init(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);

  // BraveP3ALogStore::Delegate
  std::string Serialize(base::StringPiece histogram_name,
                        uint64_t value,
                        const std::string& upload_type) override;

  // May be accessed from multiple threads, so this is thread-safe.
  bool IsActualMetric(base::StringPiece histogram_name) const override;

  // Invoked by callbacks registered by our service. Since these callbacks
  // can fire on any thread, this method reposts everything to UI thread.
  void OnHistogramChanged(const char* histogram_name,
                          uint64_t name_hash,
                          base::HistogramBase::Sample sample);

 private:
  friend class base::RefCountedThreadSafe<BraveP3AService>;
  ~BraveP3AService() override;

  void LoadDynamicMetrics();

  void MaybeOverrideSettingsFromCommandLine();

  void InitMessageMeta();

  // Updates things that change over time: week of survey, etc.
  void UpdateMessageMeta();

  void StartScheduledUpload(MetricLogType log_type);

  void OnHistogramChangedOnUI(const char* histogram_name,
                              base::HistogramBase::Sample sample,
                              size_t bucket);

  // Updates or removes a metric from the log.
  void HandleHistogramChange(base::StringPiece histogram_name, size_t bucket);

  void OnLogUploadComplete(int response_code,
                           int error_code,
                           bool was_https,
                           MetricLogType log_type);

  void DoRotationAtInitIfNeeded(MetricLogType log_type);

  // Restart the uploading process (i.e. mark all values as unsent).
  void DoRotation(MetricLogType log_type);
  void UpdateRotationTimer(MetricLogType log_type);

  // General prefs:
  bool initialized_ = false;
  PrefService* local_state_ = nullptr;

  const std::string channel_;
  const std::string week_of_install_;

  // The average interval between uploading different values.
  base::TimeDelta average_upload_interval_;
  bool randomize_upload_interval_ = true;
  // Interval between rotations, only used for testing from the command line.
  // "Typical" is for weekly metrics, "express" is for daily metrics (i.e. NTP
  // SI)
  GURL upload_server_url_;

  MessageMetainfo message_meta_;

  base::flat_map<MetricLogType, base::TimeDelta> rotation_intervals_;
  base::flat_map<MetricLogType, std::unique_ptr<BraveP3ALogStore>> log_stores_;
  base::flat_map<MetricLogType, std::unique_ptr<BraveP3AScheduler>>
      upload_schedulers_;

  // Once fired we restart the overall uploading process.
  base::flat_map<MetricLogType, std::unique_ptr<base::WallClockTimer>>
      rotation_timers_;

  // Contains metrics added via `RegisterDynamicMetric`
  base::flat_map<std::string, MetricLogType> dynamic_metric_log_types_;
  base::flat_map<
      std::string,
      std::unique_ptr<base::StatisticsRecorder::ScopedHistogramSampleObserver>>
      dynamic_metric_sample_callbacks_;

  std::unique_ptr<BraveP3AUploader> uploader_;

  // Used to store histogram values that are produced between constructing
  // the service and its initialization.
  base::flat_map<base::StringPiece, size_t> histogram_values_;

  std::vector<
      std::unique_ptr<base::StatisticsRecorder::ScopedHistogramSampleObserver>>
      histogram_sample_callbacks_;

  // Contains callbacks registered via `RegisterRotationCallback`
  base::RepeatingCallbackList<void(bool is_express)> rotation_callbacks_;
  // Contains callbacks registered via `RegisterMetricSentCallback`
  base::RepeatingCallbackList<void(const std::string& histogram_name)>
      metric_sent_callbacks_;

  // Contains last rotation times for each metric log type.
  // Used to delay uploads for a short period after rotations.
  base::flat_map<MetricLogType, base::Time> last_rotation_times_;
};

}  // namespace brave

#endif  // BRAVE_COMPONENTS_P3A_BRAVE_P3A_SERVICE_H_
