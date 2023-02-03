/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/brave_p3a_log_store.h"

#include <vector>

#include "base/check_op.h"
#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "base/rand_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "brave/components/p3a/brave_p3a_uploader.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace brave {

namespace {
constexpr char kTypicalLogPrefName[] = "p3a.logs";
constexpr char kExpressLogPrefName[] = "p3a.logs_express";
constexpr char kSlowLogPrefName[] = "p3a.logs_slow";
constexpr char kLogValueKey[] = "value";
constexpr char kLogSentKey[] = "sent";
constexpr char kLogTimestampKey[] = "timestamp";

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

const char* GetPrefName(MetricLogType type) {
  switch (type) {
    case MetricLogType::kSlow:
      return kSlowLogPrefName;
    case MetricLogType::kTypical:
      return kTypicalLogPrefName;
    case MetricLogType::kExpress:
      return kExpressLogPrefName;
  }
}

std::string GetUploadType(const std::string& histogram_name) {
  if (base::StartsWith(histogram_name, "Brave.P2A",
                       base::CompareCase::SENSITIVE)) {
    return kP2AUploadType;
  } else if (base::StartsWith(histogram_name, kCreativeMetricPrefix,
                              base::CompareCase::SENSITIVE)) {
    return kP3ACreativeUploadType;
  }
  return kP3AUploadType;
}

}  // namespace

BraveP3ALogStore::BraveP3ALogStore(Delegate* delegate,
                                   PrefService* local_state,
                                   MetricLogType type)
    : delegate_(delegate), local_state_(local_state), type_(type) {
  DCHECK(delegate_);
  DCHECK(local_state);
}

BraveP3ALogStore::~BraveP3ALogStore() = default;

void BraveP3ALogStore::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterDictionaryPref(kExpressLogPrefName);
  registry->RegisterDictionaryPref(kTypicalLogPrefName);
  registry->RegisterDictionaryPref(kSlowLogPrefName);
}

void BraveP3ALogStore::UpdateValue(const std::string& histogram_name,
                                   uint64_t value) {
  LogEntry& entry = log_[histogram_name];
  entry.value = value;

  if (!entry.sent) {
    DCHECK(entry.sent_timestamp.is_null());
    unsent_entries_.insert(histogram_name);
  }

  // Update the persistent value.
  ScopedDictPrefUpdate update(local_state_, GetPrefName(type_));
  base::Value::Dict* log_dict = update->EnsureDict(histogram_name);
  log_dict->Set(kLogValueKey, base::NumberToString(value));
  log_dict->Set(kLogSentKey, entry.sent);
}

void BraveP3ALogStore::RemoveValueIfExists(const std::string& histogram_name) {
  DCHECK(delegate_->IsActualMetric(histogram_name));
  log_.erase(histogram_name);
  unsent_entries_.erase(histogram_name);

  // Update the persistent value.
  ScopedDictPrefUpdate(local_state_, GetPrefName(type_))
      ->Remove(histogram_name);

  if (has_staged_log() && staged_entry_key_ == histogram_name) {
    staged_entry_key_.clear();
    staged_log_.clear();
  }
}

void BraveP3ALogStore::ResetUploadStamps() {
  // Clear log entries flags.
  ScopedDictPrefUpdate update(local_state_, GetPrefName(type_));
  for (auto& pair : log_) {
    if (pair.second.sent) {
      DCHECK(!pair.second.sent_timestamp.is_null());
      DCHECK(!unsent_entries_.contains(pair.first));

      pair.second.ResetSentState();

      // Update persistent values.
      base::Value::Dict* log_dict = update->EnsureDict(pair.first);
      log_dict->Set(kLogSentKey, pair.second.sent);
      log_dict->Set(kLogTimestampKey, pair.second.sent_timestamp.ToDoubleT());
    }
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

const std::string& BraveP3ALogStore::staged_log_key() const {
  return staged_entry_key_;
}

bool BraveP3ALogStore::has_unsent_logs() const {
  return !unsent_entries_.empty();
}

bool BraveP3ALogStore::has_staged_log() const {
  return !staged_entry_key_.empty();
}

const std::string& BraveP3ALogStore::staged_log() const {
  DCHECK(!staged_entry_key_.empty());
  auto iter = log_.find(staged_entry_key_);
  DCHECK(iter != log_.end());

  return staged_log_;
}

std::string BraveP3ALogStore::staged_log_type() const {
  DCHECK(!staged_entry_key_.empty());
  auto iter = log_.find(staged_entry_key_);
  DCHECK(iter != log_.end());

  return GetUploadType(iter->first);
}

const std::string& BraveP3ALogStore::staged_log_hash() const {
  NOTREACHED();
  return staged_log_hash_;
}

const std::string& BraveP3ALogStore::staged_log_signature() const {
  NOTREACHED();
  return staged_log_signature_;
}

absl::optional<uint64_t> BraveP3ALogStore::staged_log_user_id() const {
  NOTREACHED();
  return absl::nullopt;
}

void BraveP3ALogStore::StageNextLog() {
  // Stage the next item.
  DCHECK(has_unsent_logs());
  uint64_t rand_idx = base::RandGenerator(unsent_entries_.size());
  staged_entry_key_ = *(unsent_entries_.begin() + rand_idx);
  DCHECK(!log_.find(staged_entry_key_)->second.sent);

  uint64_t staged_entry_value = log_[staged_entry_key_].value;
  staged_log_ = delegate_->Serialize(staged_entry_key_, staged_entry_value,
                                     type_, GetUploadType(staged_entry_key_));

  VLOG(2) << "BraveP3ALogStore::StageNextLog: staged " << staged_entry_key_;
}

void BraveP3ALogStore::DiscardStagedLog() {
  if (!has_staged_log()) {
    return;
  }

  // Mark previous staged log as sent.
  auto log_iter = log_.find(staged_entry_key_);
  DCHECK(log_iter != log_.end());
  log_iter->second.MarkAsSent();

  // Update the persistent value.
  ScopedDictPrefUpdate update(local_state_, GetPrefName(type_));
  base::Value::Dict* log_dict = update->EnsureDict(log_iter->first);
  log_dict->Set(kLogSentKey, log_iter->second.sent);
  log_dict->Set(kLogTimestampKey, log_iter->second.sent_timestamp.ToDoubleT());

  // Erase the entry from the unsent queue.
  auto unsent_entries_iter = unsent_entries_.find(staged_entry_key_);
  DCHECK(unsent_entries_iter != unsent_entries_.end());
  unsent_entries_.erase(unsent_entries_iter);

  staged_entry_key_.clear();
  staged_log_.clear();
}

void BraveP3ALogStore::MarkStagedLogAsSent() {}

void BraveP3ALogStore::TrimAndPersistUnsentLogs(
    bool overwrite_in_memory_store) {
  NOTREACHED();
}

void BraveP3ALogStore::LoadPersistedUnsentLogs() {
  DCHECK(log_.empty());
  DCHECK(unsent_entries_.empty());

  const char* pref_name = GetPrefName(type_);

  std::vector<std::string> metrics_to_remove;

  const base::Value::Dict& log_dict = local_state_->GetDict(pref_name);
  for (const auto [name, value] : log_dict) {
    LogEntry entry;
    // Check if the metric is obsolete.
    if (!delegate_->IsActualMetric(name)) {
      // Drop it from the local state.
      metrics_to_remove.push_back(name);
      continue;
    }
    // Value.
    const base::Value::Dict& dict = value.GetDict();
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
      entry.sent_timestamp = base::Time::FromDoubleT(*v);
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

  if (!metrics_to_remove.empty()) {
    ScopedDictPrefUpdate update(local_state_, pref_name);
    for (const std::string& name : metrics_to_remove) {
      update->Remove(name);
    }
  }
}

}  // namespace brave
