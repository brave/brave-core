/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_P2A_BRAVE_P2A_LOG_STORE_H_
#define BRAVE_COMPONENTS_P2A_BRAVE_P2A_LOG_STORE_H_

#include <string>

#include "base/containers/flat_map.h"
#include "base/containers/flat_set.h"
#include "base/strings/string_piece.h"
#include "base/time/time.h"
#include "components/metrics/log_store.h"

class PrefService;
class PrefRegistrySimple;

namespace brave {

// Stores all given values in memory and persists in prefs on the fly.
// All logs (not only unsent are persistent), and all logs could be loaded
// using |LoadPersistedUnsentLogs()|. We should fix this at some point since
// for now persisted entries never expire.
class BraveP2ALogStore : public metrics::LogStore {
 public:
  class Delegate {
   public:
    // Prepares a string representaion of an entry.
    virtual std::string Serialize(base::StringPiece histogram_name,
                                  uint64_t value) const = 0;
    // Returns false if the metric is obsolete and should be cleaned up.
    virtual bool IsActualMetric(base::StringPiece histogram_name) const = 0;
    virtual ~Delegate() {}
  };

  explicit BraveP2ALogStore(Delegate* delegate,
                            PrefService* local_state);

  // TODO(iefremov): Make parent destructor virtual?
  virtual ~BraveP2ALogStore();

  static void RegisterPrefs(PrefRegistrySimple* registry);

  void UpdateValue(
      const std::string& histogram_name,
      uint64_t value,
      uint64_t bucket_count);
  // Marks all saved values as unsent.
  void ResetUploadStamps();

  // metrics::LogStore:
  bool has_unsent_logs() const override;
  bool has_staged_log() const override;
  const std::string& staged_log() const override;
  const std::string& staged_log_hash() const override;
  const std::string& staged_log_signature() const override;
  void StageNextLog() override;
  void DiscardStagedLog() override;

  // |PersistUnsentLogs| should not be used, since we persist everything
  // on the fly.
  void PersistUnsentLogs() const override;
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
    uint64_t bucket_count = 0u;
    bool sent = false;
    base::Time sent_timestamp;  // At the moment only for debugging purposes.
  };

  const Delegate* const delegate_ = nullptr;  // Weak.
  PrefService* const local_state_ = nullptr;

  // TODO(iefremov): Try to replace with base::StringPiece?
  base::flat_map<std::string, LogEntry> log_;
  base::flat_set<std::string> unsent_entries_;

  std::string staged_entry_key_;
  std::string staged_log_;

  // Not used for now.
  std::string staged_log_hash_;
  std::string staged_log_signature_;
};

}  // namespace brave

#endif  // BRAVE_COMPONENTS_P2A_BRAVE_P2A_LOG_STORE_H_
