/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_RESOURCE_MANAGER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_RESOURCE_MANAGER_H_

#include <string>

#include "base/observer_list.h"
#include "bat/ads/internal/resources/resource_manager_observer.h"

namespace ads {

class ResourceManager final {
 public:
  ResourceManager();

  ResourceManager(const ResourceManager& other) = delete;
  ResourceManager& operator=(const ResourceManager& other) = delete;

  ResourceManager(ResourceManager&& other) noexcept = delete;
  ResourceManager& operator=(ResourceManager&& other) noexcept = delete;

  ~ResourceManager();

  static ResourceManager* GetInstance();

  static bool HasInstance();

  void AddObserver(ResourceManagerObserver* observer);
  void RemoveObserver(ResourceManagerObserver* observer);

  void UpdateResource(const std::string& id);

 private:
  void NotifyResourceDidUpdate(const std::string& id) const;

  base::ObserverList<ResourceManagerObserver> observers_;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_RESOURCE_MANAGER_H_
