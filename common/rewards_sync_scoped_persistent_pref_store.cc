/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/common/rewards_sync_scoped_persistent_pref_store.h"

#include <utility>

#include "base/values.h"

namespace {
const char kBraveSyncV2[] = "brave_sync_v2";
const char kSync[] = "sync";
}  // namespace

namespace brave {
RewardsSyncScopedPersistentPrefStore::RewardsSyncScopedPersistentPrefStore(
    PersistentPrefStore* persistent_pref_store)
    : ScopedPersistentPrefStore("brave.rewards", persistent_pref_store) {}

bool RewardsSyncScopedPersistentPrefStore::GetValue(
    const std::string& key,
    const base::Value** result) const {
  return ScopedPersistentPrefStore::GetValue(MapSync(key), result);
}

std::unique_ptr<base::DictionaryValue>
RewardsSyncScopedPersistentPrefStore::GetValues() const {
  auto values = ScopedPersistentPrefStore::GetValues();
  DCHECK(values);

  if (auto values_in_brave_sync_v2 =
          values->ExtractPath(scope_ + '.' + kBraveSyncV2)) {
    values->SetPath(kBraveSyncV2, std::move(*values_in_brave_sync_v2));
  }

  if (auto values_in_sync = values->ExtractPath(scope_ + '.' + kSync)) {
    values->SetPath(kSync, std::move(*values_in_sync));
  }

  return values;
}

void RewardsSyncScopedPersistentPrefStore::SetValue(
    const std::string& key,
    std::unique_ptr<base::Value> value,
    uint32_t flags) {
  ScopedPersistentPrefStore::SetValue(MapSync(key), std::move(value), flags);
}

void RewardsSyncScopedPersistentPrefStore::RemoveValue(const std::string& key,
                                                       uint32_t flags) {
  ScopedPersistentPrefStore::RemoveValue(MapSync(key), flags);
}

bool RewardsSyncScopedPersistentPrefStore::GetMutableValue(
    const std::string& key,
    base::Value** result) {
  return ScopedPersistentPrefStore::GetMutableValue(MapSync(key), result);
}

void RewardsSyncScopedPersistentPrefStore::ReportValueChanged(
    const std::string& key,
    uint32_t flags) {
  // if |key| starts with "brave.rewards.brave_sync_v2", or
  // "brave.rewards.sync", we remove the "brave.rewards." part
  if (!key.rfind(scope_ + '.' + kBraveSyncV2, 0) ||
      !key.rfind(scope_ + '.' + kSync, 0)) {
    for (auto& observer : observers_) {
      observer.OnPrefValueChanged(key.substr(scope_.size() + 1));
    }
  } else {
    ScopedPersistentPrefStore::ReportValueChanged(key, flags);
  }
}

void RewardsSyncScopedPersistentPrefStore::SetValueSilently(
    const std::string& key,
    std::unique_ptr<base::Value> value,
    uint32_t flags) {
  ScopedPersistentPrefStore::SetValueSilently(MapSync(key), std::move(value),
                                              flags);
}

void RewardsSyncScopedPersistentPrefStore::RemoveValuesByPrefixSilently(
    const std::string& prefix) {
  ScopedPersistentPrefStore::RemoveValuesByPrefixSilently(MapSync(prefix));
}

RewardsSyncScopedPersistentPrefStore::~RewardsSyncScopedPersistentPrefStore() =
    default;

// if |key| starts with "brave_sync_v2", or "sync", we prepend "brave.rewards."
std::string RewardsSyncScopedPersistentPrefStore::MapSync(
    const std::string& key) const {
  return (!key.rfind(kBraveSyncV2, 0) || !key.rfind(kSync, 0) ? scope_ + '.'
                                                              : "") +
         key;
}
}  // namespace brave
