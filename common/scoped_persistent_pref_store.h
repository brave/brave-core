/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMMON_SCOPED_PERSISTENT_PREF_STORE_H_
#define BRAVE_COMMON_SCOPED_PERSISTENT_PREF_STORE_H_

#include <memory>
#include <string>

#include "base/observer_list.h"
#include "components/prefs/persistent_pref_store.h"

namespace brave {
class ScopedPersistentPrefStore : public PersistentPrefStore {
 public:
  explicit ScopedPersistentPrefStore(
      std::string&& scope,
      PersistentPrefStore* persistent_pref_store);

  ScopedPersistentPrefStore(const ScopedPersistentPrefStore&) = delete;
  ScopedPersistentPrefStore& operator=(const ScopedPersistentPrefStore&) =
      delete;

  // methods of PrefStore:
  void AddObserver(PrefStore::Observer* observer) override;
  void RemoveObserver(PrefStore::Observer* observer) override;
  bool HasObservers() const override;
  bool IsInitializationComplete() const override;
  bool GetValue(const std::string& key,
                const base::Value** result) const override;
  std::unique_ptr<base::DictionaryValue> GetValues() const override;

  // methods of WriteablePrefStore:
  void SetValue(const std::string& key,
                std::unique_ptr<base::Value> value,
                uint32_t flags) override;
  void RemoveValue(const std::string& key, uint32_t flags) override;
  bool GetMutableValue(const std::string& key, base::Value** result) override;
  void ReportValueChanged(const std::string& key, uint32_t flags) override;
  void SetValueSilently(const std::string& key,
                        std::unique_ptr<base::Value> value,
                        uint32_t flags) override;
  void RemoveValuesByPrefixSilently(const std::string& prefix) override;

  // methods of PersistentPrefStore:
  bool ReadOnly() const override;
  PrefReadError GetReadError() const override;
  PrefReadError ReadPrefs() override;
  void ReadPrefsAsync(ReadErrorDelegate* error_delegate) override;
  void CommitPendingWrite(base::OnceClosure reply_callback,
                          base::OnceClosure synchronous_done_callback) override;
  void SchedulePendingLossyWrites() override;
  void ClearMutableValues() override;
  void OnStoreDeletionFromDisk() override;

 protected:
  ~ScopedPersistentPrefStore() override;

  std::string scope_;
  base::ObserverList<PrefStore::Observer, true>::Unchecked observers_;

 private:
  class ObserverAdapter;

  void OnPrefValueChanged(const std::string& key);
  void OnInitializationCompleted(bool succeeded);
  bool InScope(const std::string& key) const;

  scoped_refptr<PersistentPrefStore> persistent_pref_store_;
  std::unique_ptr<ObserverAdapter> persistent_pref_store_observer_;
};
}  // namespace brave

#endif  // BRAVE_COMMON_SCOPED_PERSISTENT_PREF_STORE_H_
