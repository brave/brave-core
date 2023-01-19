/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/ad_block_filters_provider.h"

#include <utility>

namespace brave_shields {

AdBlockFiltersProvider::AdBlockFiltersProvider() = default;

AdBlockFiltersProvider::~AdBlockFiltersProvider() = default;

void AdBlockFiltersProvider::AddObserver(
    AdBlockFiltersProvider::Observer* observer) {
  if (!observers_.HasObserver(observer))
    observers_.AddObserver(observer);
}

void AdBlockFiltersProvider::RemoveObserver(
    AdBlockFiltersProvider::Observer* observer) {
  if (observers_.HasObserver(observer))
    observers_.RemoveObserver(observer);
}

void AdBlockFiltersProvider::NotifyObservers() {
  for (auto& observer : observers_) {
    observer.OnChanged();
  }
}

void AdBlockFiltersProvider::LoadDAT(
    base::OnceCallback<void(bool deserialize, const DATFileDataBuffer& dat_buf)>
        cb) {
  LoadDATBuffer(std::move(cb));
}

base::WeakPtr<AdBlockFiltersProvider> AdBlockFiltersProvider::AsWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

}  // namespace brave_shields
