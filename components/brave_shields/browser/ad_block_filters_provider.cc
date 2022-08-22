/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/ad_block_filters_provider.h"

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

void AdBlockFiltersProvider::OnDATLoaded(bool deserialize,
                                         const DATFileDataBuffer& dat_buf) {
  for (auto& observer : observers_) {
    observer.OnDATLoaded(deserialize, dat_buf);
  }
}

void AdBlockFiltersProvider::LoadDAT(
    AdBlockFiltersProvider::Observer* observer) {
  LoadDATBuffer(base::BindOnce(&AdBlockFiltersProvider::OnLoad,
                               weak_factory_.GetWeakPtr(), observer));
}

void AdBlockFiltersProvider::OnLoad(AdBlockFiltersProvider::Observer* observer,
                                    bool deserialize,
                                    const DATFileDataBuffer& dat_buf) {
  if (observers_.HasObserver(observer)) {
    observer->OnDATLoaded(deserialize, dat_buf);
  }
}

bool AdBlockFiltersProvider::Delete() && {
  return false;
}

}  // namespace brave_shields
