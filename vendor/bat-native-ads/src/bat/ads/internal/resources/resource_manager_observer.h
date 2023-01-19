/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_RESOURCE_MANAGER_OBSERVER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_RESOURCE_MANAGER_OBSERVER_H_

#include <string>

#include "base/observer_list_types.h"

namespace ads {

class ResourceManagerObserver : public base::CheckedObserver {
 public:
  // Invoked when the resource specified by |id| has updated.
  virtual void OnResourceDidUpdate(const std::string& id) {}
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_RESOURCE_MANAGER_OBSERVER_H_
