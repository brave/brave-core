/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/ad_block_resource_provider.h"

#include <string>

namespace brave_shields {

AdBlockResourceProvider::AdBlockResourceProvider() {}

AdBlockResourceProvider::~AdBlockResourceProvider() {}

void AdBlockResourceProvider::AddObserver(
    AdBlockResourceProvider::Observer* observer) {
  if (!observers_.HasObserver(observer))
    observers_.AddObserver(observer);
}

void AdBlockResourceProvider::RemoveObserver(
    AdBlockResourceProvider::Observer* observer) {
  if (observers_.HasObserver(observer))
    observers_.RemoveObserver(observer);
}

void AdBlockResourceProvider::OnResourcesLoaded(
    const std::string& resources_json) {
  for (auto& observer : observers_) {
    observer.OnResourcesLoaded(resources_json);
  }
}

}  // namespace brave_shields
