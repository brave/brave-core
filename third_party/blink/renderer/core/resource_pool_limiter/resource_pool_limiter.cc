/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/core/resource_pool_limiter/resource_pool_limiter.h"

#include "base/notreached.h"
#include "third_party/blink/renderer/platform/weborigin/security_origin.h"
#include "third_party/blink/renderer/platform/wtf/std_lib_extras.h"
#include "third_party/blink/renderer/platform/wtf/text/string_builder.h"

namespace blink {

using ResourceType = ResourcePoolLimiter::ResourceType;

namespace {

String GetResourceIdInUse(const SecurityOrigin* origin,
                          ResourceType resource_type) {
  DCHECK(origin);
  StringBuilder string_builder;
  string_builder.AppendNumber(static_cast<int>(resource_type));
  String origin_id = origin->RegistrableDomain();
  if (origin_id.IsEmpty())
    origin_id = origin->Host();
  string_builder.Append(origin_id);
  return string_builder.ToString();
}

int GetResourceLimit(ResourceType resource_type) {
  switch (resource_type) {
    case ResourceType::kWebSocket:
      return 10;
  }
  NOTREACHED();
  return 0;
}

}  // namespace

ResourcePoolLimiter::ResourceInUseTracker::ResourceInUseTracker(
    String resource_id_in_use)
    : resource_id_in_use_(std::move(resource_id_in_use)) {}

ResourcePoolLimiter::ResourceInUseTracker::~ResourceInUseTracker() {
  ResourcePoolLimiter::GetInstance().DropResourceInUse(this);
}

// static
ResourcePoolLimiter& ResourcePoolLimiter::GetInstance() {
  // This needs to be thread-safe because ResourcePoolLimiter is supported by
  // both windows and workers.
  DEFINE_THREAD_SAFE_STATIC_LOCAL(ResourcePoolLimiter, resource_pool_limiter,
                                  ());
  return resource_pool_limiter;
}

ResourcePoolLimiter::ResourcePoolLimiter() = default;
ResourcePoolLimiter::~ResourcePoolLimiter() = default;

std::unique_ptr<ResourcePoolLimiter::ResourceInUseTracker>
ResourcePoolLimiter::IssueResourceInUseTracker(
    const SecurityOrigin* security_origin,
    ResourceType resource_type) {
  DCHECK(security_origin);
  String resource_id = GetResourceIdInUse(security_origin, resource_type);

  MutexLocker locker(resources_in_use_lock_);
  // `insert` doesn't change the value if it already exists.
  int& resource_in_use_value =
      resources_in_use_.insert(resource_id, 0).stored_value->value;
  if (resource_in_use_value >= GetResourceLimit(resource_type)) {
    return nullptr;
  }

  ++resource_in_use_value;
  return std::make_unique<ResourceInUseTracker>(resource_id.IsolatedCopy());
}

void ResourcePoolLimiter::DropResourceInUse(
    const ResourceInUseTracker* resource_in_use_tracker) {
  MutexLocker locker(resources_in_use_lock_);
  auto resource_in_use_it =
      resources_in_use_.find(resource_in_use_tracker->resource_id_in_use());
  DCHECK(resource_in_use_it != resources_in_use_.end());
  if (--resource_in_use_it->value == 0) {
    resources_in_use_.erase(resource_in_use_it);
  }
}

}  // namespace blink
