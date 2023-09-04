/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_P3A_CONSTELLATION_LOG_STORE_H_
#define BRAVE_COMPONENTS_P3A_CONSTELLATION_LOG_STORE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/containers/flat_set.h"
#include "base/memory/raw_ref.h"
#include "base/strings/string_piece.h"
#include "base/time/time.h"
#include "components/metrics/log_store.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

class PrefService;
class PrefRegistrySimple;

namespace p3a {

// Stores messages from previous epochs in memory and persists all messages in
// prefs on the fly. All messages from previous epochs could be loaded using
// |LoadPersistedUnsentLogs()|. The function will also remove epochs are exceed
// the "keep epoch count".
class ConstellationLogStore : public metrics::LogStore {
 public:
  ConstellationLogStore(PrefService& local_state, size_t keep_epoch_count);
  ~ConstellationLogStore() override;

  ConstellationLogStore(const ConstellationLogStore&) = delete;
  ConstellationLogStore& operator=(const ConstellationLogStore&) = delete;

  static void RegisterPrefs(PrefRegistrySimple* registry);

  void UpdateMessage(const std::string& histogram_name,
                     uint8_t epoch,
                     const std::string& msg);

  void SetCurrentEpoch(uint8_t current_epoch);

  // metrics::LogStore:
  bool has_unsent_logs() const override;
  bool has_staged_log() const override;
  const std::string& staged_log() const override;
  std::string staged_log_type() const;
  const std::string& staged_log_hash() const override;
  const std::string& staged_log_signature() const override;
  absl::optional<uint64_t> staged_log_user_id() const override;
  void StageNextLog() override;
  void DiscardStagedLog(base::StringPiece reason = "") override;
  void MarkStagedLogAsSent() override;

  // |TrimAndPersistUnsentLogs| should not be used, since we persist everything
  // on the fly.
  void TrimAndPersistUnsentLogs(bool overwrite_in_memory_store) override;
  // Returns early if founds malformed persisted values.
  void LoadPersistedUnsentLogs() override;

 private:
  struct LogKey {
    explicit LogKey(uint8_t epoch, const std::string& histogram_name)
        : epoch(epoch), histogram_name(histogram_name) {}
    uint8_t epoch;
    std::string histogram_name;
  };

  struct LogKeyCompare {
    bool operator()(const LogKey& lhs, const LogKey& rhs) const;
  };

  void RemoveMessageIfExists(const LogKey& key);

  const raw_ref<PrefService> local_state_;

  base::flat_map<LogKey, std::string, LogKeyCompare> log_;
  base::flat_set<LogKey, LogKeyCompare> unsent_entries_;

  std::unique_ptr<LogKey> staged_entry_key_;
  std::string staged_log_;

  // Not used for now.
  std::string staged_log_hash_;
  std::string staged_log_signature_;

  uint8_t current_epoch_;
  size_t keep_epoch_count_;
};

}  // namespace p3a

#endif  // BRAVE_COMPONENTS_P3A_CONSTELLATION_LOG_STORE_H_
