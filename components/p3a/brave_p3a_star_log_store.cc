/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/brave_p3a_star_log_store.h"

#include <tuple>

#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "base/rand_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace brave {

namespace {

constexpr char kPrefName[] = "p3a.star_logs";

}  // namespace

BraveP3AStarLogStore::BraveP3AStarLogStore(PrefService* local_state,
                                           size_t keep_epoch_count)
    : local_state_(local_state), keep_epoch_count_(keep_epoch_count) {
  DCHECK(local_state);
  DCHECK_GT(keep_epoch_count, 0U);
}

BraveP3AStarLogStore::~BraveP3AStarLogStore() {}

bool BraveP3AStarLogStore::LogKeyCompare::operator()(const LogKey& lhs,
                                                     const LogKey& rhs) const {
  return std::tie(lhs.epoch, lhs.histogram_name) <
         std::tie(rhs.epoch, rhs.histogram_name);
}

void BraveP3AStarLogStore::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterDictionaryPref(kPrefName);
}

void BraveP3AStarLogStore::UpdateMessage(const std::string& histogram_name,
                                         uint8_t epoch,
                                         const std::string& msg) {
  DictionaryPrefUpdate update(local_state_, kPrefName);
  std::string epoch_key = base::NumberToString(epoch);
  if (update->FindDictKey(epoch_key) == nullptr) {
    update->SetKey(epoch_key, base::Value(base::Value::Type::DICT));
  }
  update->FindDictKey(epoch_key)->SetStringKey(histogram_name, msg);

  if (current_epoch_ != epoch) {
    LogKey key(epoch, histogram_name);
    log_[key] = msg;
  }
}

void BraveP3AStarLogStore::RemoveMessageIfExists(const LogKey& key) {
  log_.erase(key);
  unsent_entries_.erase(key);

  // Update the persistent value.
  DictionaryPrefUpdate update(local_state_, kPrefName);
  std::string epoch_key = base::NumberToString(key.epoch);
  base::Value* epoch_dict = update->FindDictKey(epoch_key);
  if (epoch_dict != nullptr) {
    epoch_dict->RemoveKey(key.histogram_name);
  }

  if (has_staged_log() && staged_entry_key_->epoch == key.epoch &&
      staged_entry_key_->histogram_name == key.histogram_name) {
    staged_entry_key_.reset();
    staged_log_.clear();
  }
}

void BraveP3AStarLogStore::SetCurrentEpoch(uint8_t current_epoch) {
  current_epoch_ = current_epoch;
}

bool BraveP3AStarLogStore::has_unsent_logs() const {
  return !unsent_entries_.empty();
}

bool BraveP3AStarLogStore::has_staged_log() const {
  return staged_entry_key_ != nullptr;
}

const std::string& BraveP3AStarLogStore::staged_log() const {
  DCHECK(staged_entry_key_);
  DCHECK(!staged_log_.empty());

  return staged_log_;
}

std::string BraveP3AStarLogStore::staged_log_type() const {
  DCHECK(staged_entry_key_);
  if (base::StartsWith(staged_entry_key_->histogram_name, "Brave.P2A",
                       base::CompareCase::SENSITIVE)) {
    return "p2a";
  }
  return "p3a";
}

const std::string& BraveP3AStarLogStore::staged_log_hash() const {
  NOTREACHED();
  return staged_log_hash_;
}

const std::string& BraveP3AStarLogStore::staged_log_signature() const {
  NOTREACHED();
  return staged_log_signature_;
}

absl::optional<uint64_t> BraveP3AStarLogStore::staged_log_user_id() const {
  NOTREACHED();
  return absl::nullopt;
}

void BraveP3AStarLogStore::StageNextLog() {
  // Stage the next item.
  DCHECK(has_unsent_logs());
  uint64_t rand_idx = base::RandGenerator(unsent_entries_.size());
  staged_entry_key_.reset(new LogKey(*(unsent_entries_.begin() + rand_idx)));

  staged_log_ = log_.at(*staged_entry_key_);

  VLOG(2) << "BraveP3AStarLogStore::StageNextLog: staged epoch = "
          << staged_entry_key_->epoch
          << ", histogram_name = " << staged_entry_key_->histogram_name;
}

void BraveP3AStarLogStore::DiscardStagedLog() {
  if (!has_staged_log()) {
    return;
  }
  staged_entry_key_.reset();
  staged_log_.clear();
}

void BraveP3AStarLogStore::MarkStagedLogAsSent() {
  if (!has_staged_log()) {
    return;
  }
  RemoveMessageIfExists(*staged_entry_key_);
}

void BraveP3AStarLogStore::TrimAndPersistUnsentLogs() {
  NOTREACHED();
}

void BraveP3AStarLogStore::LoadPersistedUnsentLogs() {
  log_.clear();
  unsent_entries_.clear();

  DictionaryPrefUpdate update(local_state_, kPrefName);
  base::Value* list = update.Get();
  for (auto epoch_dict_item : list->DictItems()) {
    uint64_t parsed_epoch;
    if (!base::StringToUint64(epoch_dict_item.first, &parsed_epoch)) {
      continue;
    }
    uint8_t item_epoch = (uint8_t)parsed_epoch;

    if (current_epoch_ == item_epoch) {
      // Do not load/send messages from the current epoch
      continue;
    }

    if ((current_epoch_ - item_epoch) >= keep_epoch_count_) {
      // If epoch is too old, delete it
      list->RemoveKey(epoch_dict_item.first);
      continue;
    }

    const base::Value& inner_epoch_dict = epoch_dict_item.second;
    for (auto msg_item : inner_epoch_dict.DictItems()) {
      if (!msg_item.second.is_string()) {
        continue;
      }
      LogKey key(item_epoch, msg_item.first);
      log_[key] = msg_item.second.GetString();

      unsent_entries_.insert(key);
    }
  }
}

}  // namespace brave
