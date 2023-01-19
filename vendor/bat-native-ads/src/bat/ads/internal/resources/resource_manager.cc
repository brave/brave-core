/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/resources/resource_manager.h"

#include "base/check_op.h"
#include "bat/ads/internal/common/logging_util.h"

namespace ads {

namespace {
ResourceManager* g_resource_manager_instance = nullptr;
}  // namespace

ResourceManager::ResourceManager() {
  DCHECK(!g_resource_manager_instance);
  g_resource_manager_instance = this;
}

ResourceManager::~ResourceManager() {
  DCHECK_EQ(this, g_resource_manager_instance);
  g_resource_manager_instance = nullptr;
}

// static
ResourceManager* ResourceManager::GetInstance() {
  DCHECK(g_resource_manager_instance);
  return g_resource_manager_instance;
}

// static
bool ResourceManager::HasInstance() {
  return !!g_resource_manager_instance;
}

void ResourceManager::AddObserver(ResourceManagerObserver* observer) {
  DCHECK(observer);
  observers_.AddObserver(observer);
}

void ResourceManager::RemoveObserver(ResourceManagerObserver* observer) {
  DCHECK(observer);
  observers_.RemoveObserver(observer);
}

void ResourceManager::UpdateResource(const std::string& id) {
  BLOG(1, "Resource id " << id << " updated");

  NotifyResourceDidUpdate(id);
}

///////////////////////////////////////////////////////////////////////////////

void ResourceManager::NotifyResourceDidUpdate(const std::string& id) const {
  for (ResourceManagerObserver& observer : observers_) {
    observer.OnResourceDidUpdate(id);
  }
}

}  // namespace ads
