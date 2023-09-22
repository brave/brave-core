/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_P3A_MESSAGE_MANAGER_H_
#define BRAVE_COMPONENTS_P3A_MESSAGE_MANAGER_H_

#include <memory>
#include <string>
#include <string_view>

#include "base/containers/flat_map.h"
#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/raw_ref.h"
#include "base/memory/ref_counted.h"
#include "base/timer/timer.h"
#include "brave/components/p3a/metric_log_store.h"
#include "brave/components/p3a/metric_log_type.h"
#include "brave/components/p3a/p3a_message.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

class PrefService;

namespace network {
class SharedURLLoaderFactory;
}

namespace p3a {

struct P3AConfig;

class Uploader;
class ConstellationLogStore;
class RotationScheduler;
class Scheduler;
class ConstellationHelper;
class Uploader;

struct RandomnessServerInfo;

// The message manager has multiple roles related to handling/reporting metric
// values. Metric updates received upstream from the Service are stored
// in their appropriate LogStore instances. The Scheduler calls
// methods in this class via callbacks and propagates metric upload to the
// Uploader. The RotationScheduler also calls methods in this
// class to handle reporting period rotation. Constellation message preparation
// is also triggered from this class.
class MessageManager : public MetricLogStore::Delegate {
 public:
  using IsDynamicMetricRegisteredCallback =
      base::RepeatingCallback<bool(const std::string& histogram_name)>;
  class Delegate {
   public:
    virtual absl::optional<MetricLogType> GetDynamicMetricLogType(
        const std::string& histogram_name) const = 0;
    virtual void OnRotation(MetricLogType log_type, bool is_constellation) = 0;
    // A metric "cycle" is a transmission to the P3A JSON server,
    // or a Constellation preparation for the current epoch.
    virtual void OnMetricCycled(const std::string& histogram_name,
                                bool is_constellation) = 0;
    virtual ~Delegate() {}
  };
  MessageManager(PrefService& local_state,
                 const P3AConfig* config,
                 Delegate& delegate,
                 std::string channel,
                 std::string week_of_install);
  ~MessageManager() override;

  MessageManager(const MessageManager&) = delete;
  MessageManager& operator=(const MessageManager&) = delete;

  static void RegisterPrefs(PrefRegistrySimple* registry);

  void Init(scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);

  void UpdateMetricValue(std::string_view histogram_name, size_t bucket);

  void RemoveMetricValue(std::string_view histogram_name);

 private:
  void StartScheduledUpload(bool is_constellation, MetricLogType log_type);
  void StartScheduledConstellationPrep();

  MetricLogType GetLogTypeForHistogram(std::string_view histogram_name);

  void OnLogUploadComplete(bool is_ok,
                           int response_code,
                           bool is_constellation,
                           MetricLogType log_type);

  void OnNewConstellationMessage(
      std::string histogram_name,
      uint8_t epoch,
      std::unique_ptr<std::string> serialized_message);

  void OnRandomnessServerInfoReady(RandomnessServerInfo* server_info);

  // Restart the uploading process (i.e. mark all values as unsent).
  void DoJsonRotation(MetricLogType log_type);

  void DoConstellationRotation();

  // MetricLogStore::Delegate
  std::string SerializeLog(std::string_view histogram_name,
                           const uint64_t value,
                           MetricLogType log_type,
                           bool is_constellation,
                           const std::string& upload_type) override;
  bool IsActualMetric(const std::string& histogram_name) const override;
  bool IsEphemeralMetric(const std::string& histogram_name) const override;

  const raw_ref<PrefService> local_state_;

  MessageMetainfo message_meta_;

  const raw_ptr<const P3AConfig> config_;

  base::flat_map<MetricLogType, std::unique_ptr<MetricLogStore>>
      json_log_stores_;
  std::unique_ptr<MetricLogStore> constellation_prep_log_store_;
  std::unique_ptr<ConstellationLogStore> constellation_send_log_store_;

  std::unique_ptr<Uploader> uploader_;
  base::flat_map<MetricLogType, std::unique_ptr<Scheduler>>
      json_upload_schedulers_;
  std::unique_ptr<Scheduler> constellation_prep_scheduler_;
  std::unique_ptr<Scheduler> constellation_upload_scheduler_;

  std::unique_ptr<ConstellationHelper> constellation_helper_;

  std::unique_ptr<RotationScheduler> rotation_scheduler_;

  const raw_ref<Delegate> delegate_;
};

}  // namespace p3a

#endif  // BRAVE_COMPONENTS_P3A_MESSAGE_MANAGER_H_
