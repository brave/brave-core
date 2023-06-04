/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_RESOURCES_ASYNC_RESOURCE_ASYNC_HANDLER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_RESOURCES_ASYNC_RESOURCE_ASYNC_HANDLER_H_

#include "base/threading/sequence_bound.h"
#include "brave/components/brave_ads/core/internal/resources/async/resource_async_manager.h"

namespace brave_ads {

template <typename ResourceRefCountedProxy>
class ResourceAsyncHandler {
 public:
  ResourceAsyncHandler() {
    Get().AsyncCall(&ResourceRefCountedProxy::AddConsumer);
  }

  virtual ~ResourceAsyncHandler() {
    Get().AsyncCall(&ResourceRefCountedProxy::RemoveConsumer);
  }

  ResourceAsyncHandler(const ResourceAsyncHandler& proxy) = delete;
  ResourceAsyncHandler& operator=(const ResourceAsyncHandler& proxy) = delete;

  ResourceAsyncHandler(ResourceAsyncHandler&& proxy) = delete;
  ResourceAsyncHandler& operator=(ResourceAsyncHandler&& proxy) = delete;

  const base::SequenceBound<ResourceRefCountedProxy>& Get() const {
    return ResourceAsyncManager::Get().GetAsyncProxy<ResourceRefCountedProxy>();
  }
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_RESOURCES_ASYNC_RESOURCE_ASYNC_HANDLER_H_
