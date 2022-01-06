/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMMON_REWARDS_SYNC_SCOPED_PERSISTENT_PREF_STORE_H_
#define BRAVE_COMMON_REWARDS_SYNC_SCOPED_PERSISTENT_PREF_STORE_H_

#include <memory>
#include <string>

#include "base/observer_list.h"
#include "brave/common/scoped_persistent_pref_store.h"

FORWARD_DECLARE_TEST(MapSync, Input);

namespace brave {
class RewardsSyncScopedPersistentPrefStore : public ScopedPersistentPrefStore {
 public:
  RewardsSyncScopedPersistentPrefStore(
      PersistentPrefStore* persistent_pref_store);

  RewardsSyncScopedPersistentPrefStore(
      const RewardsSyncScopedPersistentPrefStore&) = delete;
  RewardsSyncScopedPersistentPrefStore& operator=(
      const RewardsSyncScopedPersistentPrefStore&) = delete;

  // methods of PrefStore:
  bool GetValue(const std::string& key,
                const base::Value** result) const override;
  std::unique_ptr<base::DictionaryValue> GetValues() const override;

  // methods of WriteablePrefStore:
  void SetValue(const std::string& key,
                std::unique_ptr<base::Value> value,
                uint32_t flags) override;
  void RemoveValue(const std::string& key, uint32_t flags) override;
  bool GetMutableValue(const std::string& key, base::Value** result) override;
  // We cannot fully support GetMutableValue(), since mutating a value
  // would require the usage of the key's mapped location, which we cannot
  // guarantee outside RewardsSyncScopedPersistentPrefStore, e.g.:
  // underlay_->SetValue(
  //     "brave.rewards.sync.bookmarks",
  //     std::make_unique<base::Value>(base::Value::Type::DICTIONARY),
  //     WriteablePrefStore::DEFAULT_PREF_WRITE_FLAGS);
  // base::Value* value = nullptr;
  // EXPECT_TRUE(overlay_->GetMutableValue("sync.bookmarks", &value));
  // ASSERT_TRUE(value->is_dict());
  // static_cast<base::DictionaryValue*>(value)
  //     ->SetBoolean("sync.bookmarks", true);
  //       ^ writing to sync directly
  void ReportValueChanged(const std::string& key, uint32_t flags) override;
  void SetValueSilently(const std::string& key,
                        std::unique_ptr<base::Value> value,
                        uint32_t flags) override;
  void RemoveValuesByPrefixSilently(const std::string& prefix) override;

 protected:
  ~RewardsSyncScopedPersistentPrefStore() override;

 private:
  std::string MapSync(const std::string& key) const;

  FRIEND_TEST_ALL_PREFIXES(::MapSync, Input);
};
}  // namespace brave

#endif  // BRAVE_COMMON_REWARDS_SYNC_SCOPED_PERSISTENT_PREF_STORE_H_
