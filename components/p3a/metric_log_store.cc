/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/metric_log_store.h"

#include <optional>
#include <string_view>
#include <vector>

#include "base/check.h"
#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "base/notreached.h"
#include "base/rand_util.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/p3a/metric_log_type.h"
#include "brave/components/p3a/pref_names.h"
#include "brave/components/p3a/uploader.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace p3a {

namespace {

inline constexpr char kLogValueKey[] = "value";
inline constexpr char kLogSentKey[] = "sent";
inline constexpr char kLogTimestampKey[] = "timestamp";

void RecordSentAnswersCount(uint64_t answers_count) {
  int answer = 0;
  if (1 <= answers_count && answers_count < 5) {
    answer = 1;
  } else if (5 <= answers_count && answers_count < 10) {
    answer = 2;
  } else if (10 <= answers_count) {
    answer = 3;
  }
  UMA_HISTOGRAM_EXACT_LINEAR("Brave.P3A.SentAnswersCount", answer, 3);
}

bool IsMetricCreative(const std::string& histogram_name) {
  return histogram_name.starts_with(kCreativeMetricPrefix) ||
         histogram_name.starts_with(kCampaignMetricPrefix);
}

}  // namespace

std::string GetUploadType(const std::string& histogram_name) {
  if (IsMetricCreative(histogram_name)) {
    return kP3ACreativeUploadType;
  }
  return kP3AUploadType;
}

MetricLogStore::MetricLogStore(Delegate& delegate,
                               PrefService& local_state,
                               MetricLogType type)
    : delegate_(delegate), local_state_(local_state), type_(type) {}

MetricLogStore::~MetricLogStore() = default;

void MetricLogStore::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterDictionaryPref(kTypicalConstellationPrepPrefName);
  registry->RegisterDictionaryPref(kExpressConstellationPrepPrefName);
  registry->RegisterDictionaryPref(kSlowConstellationPrepPrefName);
}

void MetricLogStore::RegisterLocalStatePrefsForMigration(
    PrefRegistrySimple* registry) {
  // Added 06/2025
  registry->RegisterDictionaryPref(kTypicalJsonLogPrefName);
  registry->RegisterDictionaryPref(kExpressJsonLogPrefName);
  registry->RegisterDictionaryPref(kSlowJsonLogPrefName);
}

void MetricLogStore::MigrateObsoleteLocalStatePrefs(PrefService* local_state) {
  local_state->ClearPref(kTypicalJsonLogPrefName);
  local_state->ClearPref(kExpressJsonLogPrefName);
  local_state->ClearPref(kSlowJsonLogPrefName);
}

const char* MetricLogStore::GetPrefName() const {
  switch (type_) {
    case MetricLogType::kTypical:
      return kTypicalConstellationPrepPrefName;
    case MetricLogType::kExpress:
      return kExpressConstellationPrepPrefName;
    case MetricLogType::kSlow:
      return kSlowConstellationPrepPrefName;
  }
}

void MetricLogStore::UpdateValue(const std::string& histogram_name,
                                 uint64_t value) {
  LogEntry& entry = log_[histogram_name];
  entry.value = value;

  if (!entry.sent) {
    DCHECK(entry.sent_timestamp.is_null());
    unsent_entries_.insert(histogram_name);
  }

  // Update the persistent value.
  ScopedDictPrefUpdate update(&*local_state_, GetPrefName());
  base::DictValue* log_dict = update->EnsureDict(histogram_name);
  log_dict->Set(kLogValueKey, base::NumberToString(value));
  log_dict->Set(kLogSentKey, entry.sent);
}

void MetricLogStore::RemoveValueIfExists(const std::string& histogram_name) {
  log_.erase(histogram_name);
  unsent_entries_.erase(histogram_name);

  // Update the persistent value.
  ScopedDictPrefUpdate(&*local_state_, GetPrefName())->Remove(histogram_name);

  if (has_staged_log() && staged_entry_key_ == histogram_name) {
    staged_entry_key_.clear();
    staged_log_.clear();
  }
}

void MetricLogStore::ResetUploadStamps() {
  // Clear log entries flags.
  ScopedDictPrefUpdate update(&*local_state_, GetPrefName());
  for (auto it = log_.begin(); it != log_.end();) {
    if (it->second.sent) {
      DCHECK(!it->second.sent_timestamp.is_null());
      DCHECK(!unsent_entries_.contains(it->first));

      auto metric_log_type = delegate_->GetLogTypeForHistogram(it->first);
      if (!metric_log_type || metric_log_type != type_ ||
          delegate_->IsEphemeralMetric(it->first)) {
        // Ephemeral metrics should only be sent once.
        // Remove value from log store so it doesn't get
        // sent again (unless another histogram value is recorded)
        update->Remove(it->first);
        it = log_.erase(it);
        continue;
      }

      it->second.ResetSentState();

      // Update persistent values.
      base::DictValue* log_dict = update->EnsureDict(it->first);
      log_dict->Set(kLogSentKey, it->second.sent);
      log_dict->Set(kLogTimestampKey,
                    it->second.sent_timestamp.InSecondsFSinceUnixEpoch());
    }
    it++;
  }

  // Only record the sent answers count metric for weekly metrics
  if (type_ == MetricLogType::kTypical) {
    RecordSentAnswersCount(log_.size() - unsent_entries_.size());
  }

  // Rebuild the unsent set.
  unsent_entries_.clear();
  for (const auto& pair : log_) {
    unsent_entries_.insert(pair.first);
  }
}

bool MetricLogStore::has_unsent_logs() const {
  return !unsent_entries_.empty();
}

bool MetricLogStore::has_staged_log() const {
  return !staged_entry_key_.empty();
}

const std::string& MetricLogStore::staged_log() const {
  DCHECK(!staged_entry_key_.empty());
  auto iter = log_.find(staged_entry_key_);
  DCHECK(iter != log_.end());

  return staged_log_;
}

std::string MetricLogStore::staged_log_type() const {
  DCHECK(!staged_entry_key_.empty());
  auto iter = log_.find(staged_entry_key_);
  DCHECK(iter != log_.end());

  return GetUploadType(iter->first);
}

const std::string& MetricLogStore::staged_log_key() const {
  DCHECK(!staged_entry_key_.empty());

  return staged_entry_key_;
}

const std::string& MetricLogStore::staged_log_hash() const {
  NOTREACHED();
}

const std::string& MetricLogStore::staged_log_signature() const {
  NOTREACHED();
}

std::optional<uint64_t> MetricLogStore::staged_log_user_id() const {
  NOTREACHED();
}

void MetricLogStore::StageNextLog() {
  // Stage the next item.
  DCHECK(has_unsent_logs());
  uint64_t rand_idx = base::RandGenerator(unsent_entries_.size());
  staged_entry_key_ = *(unsent_entries_.begin() + rand_idx);
  DCHECK(!log_.find(staged_entry_key_)->second.sent);

  uint64_t staged_entry_value = log_[staged_entry_key_].value;
  staged_log_ =
      delegate_->SerializeLog(staged_entry_key_, staged_entry_value, type_,
                              GetUploadType(staged_entry_key_));

  VLOG(2) << "MetricLogStore::StageNextLog: staged " << staged_entry_key_;
}

void MetricLogStore::DiscardStagedLog(std::string_view reason) {
  if (!has_staged_log()) {
    return;
  }

  // Mark previous staged log as sent.
  auto log_iter = log_.find(staged_entry_key_);
  DCHECK(log_iter != log_.end());
  log_iter->second.MarkAsSent();

  // Update the persistent value.
  ScopedDictPrefUpdate update(&*local_state_, GetPrefName());
  base::DictValue* log_dict = update->EnsureDict(log_iter->first);
  log_dict->Set(kLogSentKey, log_iter->second.sent);
  log_dict->Set(kLogTimestampKey,
                log_iter->second.sent_timestamp.InSecondsFSinceUnixEpoch());

  // Erase the entry from the unsent queue.
  auto unsent_entries_iter = unsent_entries_.find(staged_entry_key_);
  DCHECK(unsent_entries_iter != unsent_entries_.end());
  unsent_entries_.erase(unsent_entries_iter);

  staged_entry_key_.clear();
  staged_log_.clear();
}

void MetricLogStore::MarkStagedLogAsSent() {}

void MetricLogStore::TrimAndPersistUnsentLogs(bool overwrite_in_memory_store) {
  NOTREACHED();
}

void MetricLogStore::LoadPersistedUnsentLogs() {
  DCHECK(log_.empty());
  DCHECK(unsent_entries_.empty());

  const char* pref_name = GetPrefName();

  const base::DictValue& log_dict = local_state_->GetDict(pref_name);
  for (const auto [name, value] : log_dict) {
    LogEntry entry;
    const base::DictValue& dict = value.GetDict();
    if (const std::string* v = dict.FindString(kLogValueKey)) {
      if (!base::StringToUint64(*v, &entry.value)) {
        return;
      }
    } else {
      return;
    }

    // Sent flag.
    if (auto v = dict.FindBool(kLogSentKey)) {
      entry.sent = *v;
    } else {
      return;
    }

    // Timestamp.
    if (auto v = dict.FindDouble(kLogTimestampKey)) {
      entry.sent_timestamp = base::Time::FromSecondsSinceUnixEpoch(*v);
      if ((entry.sent && entry.sent_timestamp.is_null()) ||
          (!entry.sent && !entry.sent_timestamp.is_null())) {
        return;
      }
    }

    log_[name] = entry;
    if (!entry.sent) {
      unsent_entries_.insert(name);
    }
  }
}

void MetricLogStore::RemoveObsoleteLogs() {
  std::vector<std::string> metrics_to_remove;
  for (const auto& [name, entry] : log_) {
    auto metric_log_type = delegate_->GetLogTypeForHistogram(name);
    if (!metric_log_type || *metric_log_type != type_) {
      metrics_to_remove.push_back(name);
    }
  }
  for (const auto& name : metrics_to_remove) {
    RemoveValueIfExists(name);
  }
}

const metrics::LogMetadata MetricLogStore::staged_log_metadata() const {
  DCHECK(has_staged_log());
  return {};
}

}  // namespace p3a
