/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/resources/async/resource_async_manager.h"

#include "base/no_destructor.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"

namespace brave_ads {

namespace {

const ResourceAsyncManager* g_async_resource_manager_for_testing = nullptr;

}  // namespace

ResourceAsyncManager::ResourceAsyncManager()
    : task_runner_(
          base::ThreadPool::CreateSequencedTaskRunner({base::MayBlock()})),
      text_processing_async_proxy_(task_runner_),
      embedding_processing_async_proxy_(task_runner_) {}

ResourceAsyncManager::~ResourceAsyncManager() = default;

template <>
const base::SequenceBound<TextProcessingRefCountedProxy>&
ResourceAsyncManager::GetAsyncProxy() const {
  return text_processing_async_proxy_;
}

template <>
const base::SequenceBound<EmbeddingProcessingRefCountedProxy>&
ResourceAsyncManager::GetAsyncProxy() const {
  return embedding_processing_async_proxy_;
}

// static
const ResourceAsyncManager& ResourceAsyncManager::Get() {
  static const base::NoDestructor<ResourceAsyncManager> kAyncResourceManager;

  if (g_async_resource_manager_for_testing) {
    return *g_async_resource_manager_for_testing;
  }

  return *kAyncResourceManager;
}

ScopedResourceAsyncManagerForTesting::ScopedResourceAsyncManagerForTesting() {
  g_async_resource_manager_for_testing = &manager_;
}

ScopedResourceAsyncManagerForTesting::~ScopedResourceAsyncManagerForTesting() {
  g_async_resource_manager_for_testing = nullptr;
}

}  // namespace brave_ads
