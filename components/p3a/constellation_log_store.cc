/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/constellation_log_store.h"

#include <memory>
#include <optional>
#include <set>
#include <string_view>

#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "base/rand_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "brave/components/p3a/metric_log_store.h"
#include "brave/components/p3a/uploader.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace p3a {

namespace {

constexpr char kTypicalPrefName[] = "p3a.constellation_logs";
constexpr char kSlowPrefName[] = "p3a.constellation_logs_slow";
constexpr char kExpressV1PrefName[] = "p3a.constellation_logs_express";
constexpr char kExpressV2PrefName[] = "p3a.constellation_logs_express_v2";

}  // namespace

const size_t kTypicalMaxEpochsToRetain = 4;
const size_t kSlowMaxEpochsToRetain = 2;
const size_t kExpressMaxEpochsToRetain = 21;

ConstellationLogStore::ConstellationLogStore(PrefService& local_state,
                                             MetricLogType log_type)
    : local_state_(local_state), log_type_(log_type) {
  local_state.ClearPref(kExpressV1PrefName);
}

ConstellationLogStore::~ConstellationLogStore() = default;

bool ConstellationLogStore::LogKeyCompare::operator()(const LogKey& lhs,
                                                      const LogKey& rhs) const {
  return std::tie(lhs.epoch, lhs.histogram_name) <
         std::tie(rhs.epoch, rhs.histogram_name);
}

void ConstellationLogStore::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterDictionaryPref(kTypicalPrefName);
  registry->RegisterDictionaryPref(kSlowPrefName);
  registry->RegisterDictionaryPref(kExpressV2PrefName);
  // Following pref is deprecated, added 12/2023
  // TODO(djandries): remove by the end of Q1 2024
  registry->RegisterDictionaryPref(kExpressV1PrefName);
}

const char* ConstellationLogStore::GetPrefName() const {
  switch (log_type_) {
    case MetricLogType::kTypical:
      return kTypicalPrefName;
    case MetricLogType::kExpress:
      return kExpressV2PrefName;
    case MetricLogType::kSlow:
      return kSlowPrefName;
  }
}

void ConstellationLogStore::UpdateMessage(const std::string& histogram_name,
                                          uint8_t epoch,
                                          const std::string& msg) {
  ScopedDictPrefUpdate update(&*local_state_, GetPrefName());
  std::string epoch_key = base::NumberToString(epoch);
  base::Value::Dict* epoch_dict = update->EnsureDict(epoch_key);
  epoch_dict->Set(histogram_name, msg);

  LogKey key(epoch, histogram_name);
  log_[key] = msg;
  unsent_entries_.insert(key);
}

void ConstellationLogStore::RemoveMessageIfExists(const LogKey& key) {
  log_.erase(key);
  unsent_entries_.erase(key);

  // Update the persistent value.
  ScopedDictPrefUpdate update(&*local_state_, GetPrefName());
  std::string epoch_key = base::NumberToString(key.epoch);
  base::Value::Dict* epoch_dict = update->EnsureDict(epoch_key);
  epoch_dict->Remove(key.histogram_name);

  if (has_staged_log() && staged_entry_key_->epoch == key.epoch &&
      staged_entry_key_->histogram_name == key.histogram_name) {
    staged_entry_key_ = nullptr;
    staged_log_.clear();
  }
}

void ConstellationLogStore::SetCurrentEpoch(uint8_t current_epoch) {
  current_epoch_ = current_epoch;
}

bool ConstellationLogStore::has_unsent_logs() const {
  return !unsent_entries_.empty();
}

bool ConstellationLogStore::has_staged_log() const {
  return staged_entry_key_ != nullptr;
}

const std::string& ConstellationLogStore::staged_log() const {
  DCHECK(staged_entry_key_);
  DCHECK(!staged_log_.empty());

  return staged_log_;
}

std::string ConstellationLogStore::staged_log_type() const {
  DCHECK(staged_entry_key_);
  return GetUploadType(staged_entry_key_->histogram_name);
}

const std::string& ConstellationLogStore::staged_log_histogram_name() const {
  DCHECK(staged_entry_key_);
  return staged_entry_key_->histogram_name;
}

const std::string& ConstellationLogStore::staged_log_hash() const {
  NOTREACHED();
}

const std::string& ConstellationLogStore::staged_log_signature() const {
  NOTREACHED();
}

std::optional<uint64_t> ConstellationLogStore::staged_log_user_id() const {
  NOTREACHED();
}

void ConstellationLogStore::StageNextLog() {
  // Stage the next item.
  DCHECK(has_unsent_logs());
  uint64_t rand_idx = base::RandGenerator(unsent_entries_.size());
  staged_entry_key_ =
      std::make_unique<LogKey>(*(unsent_entries_.begin() + rand_idx));

  staged_log_ = log_.at(*staged_entry_key_);

  VLOG(2) << "ConstellationLogStore::StageNextLog: staged epoch = "
          << static_cast<int>(staged_entry_key_->epoch)
          << ", histogram_name = " << staged_entry_key_->histogram_name;
}

void ConstellationLogStore::DiscardStagedLog(std::string_view reason) {
  if (!has_staged_log()) {
    return;
  }
  staged_entry_key_ = nullptr;
  staged_log_.clear();
}

void ConstellationLogStore::MarkStagedLogAsSent() {
  if (!has_staged_log()) {
    return;
  }
  RemoveMessageIfExists(*staged_entry_key_);
}

void ConstellationLogStore::TrimAndPersistUnsentLogs(
    bool overwrite_in_memory_store) {
  NOTREACHED();
}

size_t ConstellationLogStore::GetMaxEpochsToRetain() const {
  switch (log_type_) {
    case MetricLogType::kTypical:
      return kTypicalMaxEpochsToRetain;
    case MetricLogType::kExpress:
      return kExpressMaxEpochsToRetain;
    case MetricLogType::kSlow:
      return kSlowMaxEpochsToRetain;
  }
}

void ConstellationLogStore::LoadPersistedUnsentLogs() {
  log_.clear();
  unsent_entries_.clear();

  std::vector<std::string> epochs_to_remove;

  const base::Value::Dict& log_dict = local_state_->GetDict(GetPrefName());
  for (const auto [epoch_key, inner_epoch_dict] : log_dict) {
    uint64_t parsed_epoch;
    if (!base::StringToUint64(epoch_key, &parsed_epoch)) {
      continue;
    }
    uint8_t item_epoch = (uint8_t)parsed_epoch;

    if ((current_epoch_ - item_epoch) >= GetMaxEpochsToRetain()) {
      // If epoch is too old, delete it
      epochs_to_remove.push_back(epoch_key);
      continue;
    }

    for (const auto [histogram_name, log_value] : inner_epoch_dict.GetDict()) {
      if (!log_value.is_string()) {
        continue;
      }
      LogKey key(item_epoch, histogram_name);
      log_[key] = log_value.GetString();

      unsent_entries_.insert(key);
    }
  }

  if (!epochs_to_remove.empty()) {
    ScopedDictPrefUpdate update(&*local_state_, GetPrefName());
    for (const std::string& epoch : epochs_to_remove) {
      update->Remove(epoch);
    }
  }
}

const metrics::LogMetadata ConstellationLogStore::staged_log_metadata() const {
  return {};
}

}  // namespace p3a
