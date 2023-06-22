/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_RESOURCES_ASYNC_RESOURCE_ASYNC_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_RESOURCES_ASYNC_RESOURCE_ASYNC_MANAGER_H_

#include "base/threading/sequence_bound.h"
#include "brave/components/brave_ads/core/internal/resources/contextual/text_classification/text_processing_ref_counted_proxy.h"
#include "brave/components/brave_ads/core/internal/resources/contextual/text_embedding/embedding_processing_ref_counted_proxy.h"

namespace brave_ads {

class ResourceAsyncManager final {
 public:
  ResourceAsyncManager();

  ~ResourceAsyncManager();

  ResourceAsyncManager(const ResourceAsyncManager& proxy) = delete;
  ResourceAsyncManager& operator=(const ResourceAsyncManager& proxy) = delete;

  ResourceAsyncManager(ResourceAsyncManager&& proxy) = delete;
  ResourceAsyncManager& operator=(ResourceAsyncManager&& proxy) = delete;

  template <typename ResourceRefCountedProxy>
  const base::SequenceBound<ResourceRefCountedProxy>& GetAsyncProxy() const;

 private:
  template <typename T>
  friend class ResourceAsyncHandler;

  // Should be used by ResourceAsyncHandler only.
  static const ResourceAsyncManager& Get();

  scoped_refptr<base::SequencedTaskRunner> task_runner_;

  const base::SequenceBound<TextProcessingRefCountedProxy>
      text_processing_async_proxy_;

  const base::SequenceBound<EmbeddingProcessingRefCountedProxy>
      embedding_processing_async_proxy_;
};

class ScopedResourceAsyncManagerForTesting {
 public:
  ScopedResourceAsyncManagerForTesting();
  ~ScopedResourceAsyncManagerForTesting();

 private:
  ResourceAsyncManager manager_;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_RESOURCES_ASYNC_RESOURCE_ASYNC_MANAGER_H_
