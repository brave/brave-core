/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/ad_block_source_provider.h"

namespace brave_shields {

AdBlockSourceProvider::AdBlockSourceProvider() {}

AdBlockSourceProvider::~AdBlockSourceProvider() {}

void AdBlockSourceProvider::AddObserver(
    AdBlockSourceProvider::Observer* observer) {
  observers_.AddObserver(observer);
}

void AdBlockSourceProvider::RemoveObserver(
    AdBlockSourceProvider::Observer* observer) {
  observers_.RemoveObserver(observer);
}

void AdBlockSourceProvider::OnDATLoaded(const DATFileDataBuffer& dat_buf) {
  for (auto& observer : observers_) {
    observer.OnDATLoaded(dat_buf);
  }
}

void AdBlockSourceProvider::OnListSourceLoaded(
    const DATFileDataBuffer& list_source) {
  for (auto& observer : observers_) {
    observer.OnListSourceLoaded(list_source);
  }
}

}  // namespace brave_shields
