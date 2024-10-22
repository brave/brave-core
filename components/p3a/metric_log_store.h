/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_P3A_METRIC_LOG_STORE_H_
#define BRAVE_COMPONENTS_P3A_METRIC_LOG_STORE_H_

#include <optional>
#include <string>
#include <string_view>

#include "base/containers/flat_map.h"
#include "base/containers/flat_set.h"
#include "base/memory/raw_ref.h"
#include "base/time/time.h"
#include "brave/components/p3a/metric_log_type.h"
#include "components/metrics/log_store.h"

class PrefService;
class PrefRegistrySimple;

namespace p3a {

std::string GetUploadType(const std::string& histogram_name);

// Stores all given values in memory and persists in prefs on the fly.
// All logs (not only unsent are persistent), and all logs could be loaded
// using |LoadPersistedUnsentLogs()|. We should fix this at some point since
// for now persisted entries never expire.
class MetricLogStore : public metrics::LogStore {
 public:
  class Delegate {
   public:
    // Prepares a string representaion of an entry.
    virtual std::string SerializeLog(std::string_view histogram_name,
                                     uint64_t value,
                                     MetricLogType log_type,
                                     bool is_constellation,
                                     const std::string& upload_type) = 0;
    // Returns false if the metric is obsolete and should be cleaned up.
    virtual bool IsActualMetric(const std::string& histogram_name) const = 0;
    virtual bool IsEphemeralMetric(const std::string& histogram_name) const = 0;
    virtual ~Delegate() {}
  };

  MetricLogStore(Delegate& delegate,
                 PrefService& local_state,
                 bool is_constellation,
                 MetricLogType type);
  ~MetricLogStore() override;

  MetricLogStore(const MetricLogStore&) = delete;
  MetricLogStore& operator=(const MetricLogStore&) = delete;

  static void RegisterPrefs(PrefRegistrySimple* registry);

  void UpdateValue(const std::string& histogram_name, uint64_t value);
  // Removes and also unstages the metric value if it is known and/or staged.
  void RemoveValueIfExists(const std::string& histogram_name);
  // Marks all saved values as unsent.
  void ResetUploadStamps();

  // metrics::LogStore:
  bool has_unsent_logs() const override;
  bool has_staged_log() const override;
  const std::string& staged_log() const override;
  std::string staged_log_type() const;
  const std::string& staged_log_key() const;
  const std::string& staged_log_hash() const override;
  const std::string& staged_log_signature() const override;
  std::optional<uint64_t> staged_log_user_id() const override;
  void StageNextLog() override;
  void DiscardStagedLog(std::string_view reason = "") override;
  void MarkStagedLogAsSent() override;
  const metrics::LogMetadata staged_log_metadata() const override;

  // |TrimAndPersistUnsentLogs| should not be used, since we persist everything
  // on the fly.
  void TrimAndPersistUnsentLogs(bool overwrite_in_memory_store) override;
  // Returns early if founds malformed persisted values.
  void LoadPersistedUnsentLogs() override;

 private:
  struct LogEntry {
    LogEntry() {}
    explicit LogEntry(size_t value) : value(value) {}
    void ResetSentState() {
      sent = false;
      sent_timestamp = {};
    }
    void MarkAsSent() {
      sent = true;
      sent_timestamp = base::Time::Now();
    }

    uint64_t value = 0u;
    bool sent = false;
    base::Time sent_timestamp;  // At the moment only for debugging purposes.
  };

  const char* GetPrefName() const;

  const raw_ref<Delegate> delegate_;
  const raw_ref<PrefService, DanglingUntriaged> local_state_;

  MetricLogType type_;

  // TODO(iefremov): Try to replace with std::string_view?
  base::flat_map<std::string, LogEntry> log_;
  base::flat_set<std::string> unsent_entries_;

  std::string staged_entry_key_;
  std::string staged_log_;

  // Not used for now.
  std::string staged_log_hash_;
  std::string staged_log_signature_;

  bool is_constellation_;
};

}  // namespace p3a

#endif  // BRAVE_COMPONENTS_P3A_METRIC_LOG_STORE_H_
