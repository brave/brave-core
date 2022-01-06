/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/common/scoped_persistent_pref_store.h"

#include <utility>

#include "base/values.h"

namespace brave {
class ScopedPersistentPrefStore::ObserverAdapter : public PrefStore::Observer {
 public:
  explicit ObserverAdapter(ScopedPersistentPrefStore* parent)
      : parent_(parent) {}

  // methods of PrefStore::Observer:
  void OnPrefValueChanged(const std::string& key) override {
    parent_->OnPrefValueChanged(key);
  }

  void OnInitializationCompleted(bool succeeded) override {
    parent_->OnInitializationCompleted(succeeded);
  }

 private:
  ScopedPersistentPrefStore* parent_;
};

ScopedPersistentPrefStore::ScopedPersistentPrefStore(
    std::string&& scope,
    PersistentPrefStore* persistent_pref_store)
    : scope_((DCHECK(!scope.empty()) << "scope is empty. Consider using an "
                                        "ordinary PersistentPrefStore!",
              std::move(scope))),
      persistent_pref_store_(
          (DCHECK(persistent_pref_store), persistent_pref_store)),
      persistent_pref_store_observer_(
          std::make_unique<ScopedPersistentPrefStore::ObserverAdapter>(this)) {
  persistent_pref_store_->AddObserver(persistent_pref_store_observer_.get());
}

void ScopedPersistentPrefStore::AddObserver(PrefStore::Observer* observer) {
  observers_.AddObserver(observer);
}

void ScopedPersistentPrefStore::RemoveObserver(PrefStore::Observer* observer) {
  observers_.RemoveObserver(observer);
}

bool ScopedPersistentPrefStore::HasObservers() const {
  return !observers_.empty();
}

bool ScopedPersistentPrefStore::IsInitializationComplete() const {
  return persistent_pref_store_->IsInitializationComplete();
}

bool ScopedPersistentPrefStore::GetValue(const std::string& key,
                                         const base::Value** result) const {
  return InScope(key) && persistent_pref_store_->GetValue(key, result);
}

std::unique_ptr<base::DictionaryValue> ScopedPersistentPrefStore::GetValues()
    const {
  auto values = std::make_unique<base::DictionaryValue>();

  auto underlying_values = persistent_pref_store_->GetValues();
  DCHECK(underlying_values);

  if (auto values_in_scope = underlying_values->ExtractPath(scope_)) {
    values->SetPath(scope_, std::move(*values_in_scope));
  }

  return values;
}

void ScopedPersistentPrefStore::SetValue(const std::string& key,
                                         std::unique_ptr<base::Value> value,
                                         uint32_t flags) {
  if (InScope(key)) {
    persistent_pref_store_->SetValue(key, std::move(value), flags);
  }
}

void ScopedPersistentPrefStore::RemoveValue(const std::string& key,
                                            uint32_t flags) {
  if (InScope(key)) {
    persistent_pref_store_->RemoveValue(key, flags);
  }
}

bool ScopedPersistentPrefStore::GetMutableValue(const std::string& key,
                                                base::Value** result) {
  return InScope(key) && persistent_pref_store_->GetMutableValue(key, result);
}

void ScopedPersistentPrefStore::ReportValueChanged(const std::string& key,
                                                   uint32_t) {
  if (InScope(key)) {
    for (auto& observer : observers_) {
      observer.OnPrefValueChanged(key);
    }
  }
}

void ScopedPersistentPrefStore::SetValueSilently(
    const std::string& key,
    std::unique_ptr<base::Value> value,
    uint32_t flags) {
  if (InScope(key)) {
    persistent_pref_store_->SetValueSilently(key, std::move(value), flags);
  }
}

void ScopedPersistentPrefStore::RemoveValuesByPrefixSilently(
    const std::string& prefix) {
  if (InScope(prefix)) {
    persistent_pref_store_->RemoveValuesByPrefixSilently(prefix);
  }
}

bool ScopedPersistentPrefStore::ReadOnly() const {
  return persistent_pref_store_->ReadOnly();
}

PersistentPrefStore::PrefReadError ScopedPersistentPrefStore::GetReadError()
    const {
  return persistent_pref_store_->GetReadError();
}

PersistentPrefStore::PrefReadError ScopedPersistentPrefStore::ReadPrefs() {
  return persistent_pref_store_->ReadPrefs();
}

void ScopedPersistentPrefStore::ReadPrefsAsync(
    ReadErrorDelegate* error_delegate) {
  persistent_pref_store_->ReadPrefsAsync(error_delegate);
}

void ScopedPersistentPrefStore::CommitPendingWrite(
    base::OnceClosure reply_callback,
    base::OnceClosure synchronous_done_callback) {
  persistent_pref_store_->CommitPendingWrite(
      std::move(reply_callback), std::move(synchronous_done_callback));
}

void ScopedPersistentPrefStore::SchedulePendingLossyWrites() {
  persistent_pref_store_->SchedulePendingLossyWrites();
}

void ScopedPersistentPrefStore::ClearMutableValues() {
  persistent_pref_store_->ClearMutableValues();
}

void ScopedPersistentPrefStore::OnStoreDeletionFromDisk() {
  persistent_pref_store_->OnStoreDeletionFromDisk();
}

ScopedPersistentPrefStore::~ScopedPersistentPrefStore() {
  persistent_pref_store_->RemoveObserver(persistent_pref_store_observer_.get());
}

void ScopedPersistentPrefStore::OnPrefValueChanged(const std::string& key) {
  ReportValueChanged(key, DEFAULT_PREF_WRITE_FLAGS);
}

void ScopedPersistentPrefStore::OnInitializationCompleted(bool succeeded) {
  for (auto& observer : observers_) {
    observer.OnInitializationCompleted(succeeded);
  }
}

// returns if |key| starts with |scope_|
bool ScopedPersistentPrefStore::InScope(const std::string& key) const {
  return !key.rfind(scope_, 0);
}
}  // namespace brave
