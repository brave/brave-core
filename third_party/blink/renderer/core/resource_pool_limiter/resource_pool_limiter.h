/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_RESOURCE_POOL_LIMITER_RESOURCE_POOL_LIMITER_H_
#define BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_RESOURCE_POOL_LIMITER_RESOURCE_POOL_LIMITER_H_

#include <memory>
#include <utility>

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/wtf/hash_map.h"
#include "third_party/blink/renderer/platform/wtf/text/string_hash.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"
#include "third_party/blink/renderer/platform/wtf/threading_primitives.h"

namespace blink {

class ExecutionContext;

class CORE_EXPORT ResourcePoolLimiter {
 public:
  enum class ResourceType {
    kWebSocket,
  };

  class CORE_EXPORT ResourceInUseTracker {
   public:
    explicit ResourceInUseTracker(String resource_id);
    ~ResourceInUseTracker();

    const String& resource_id() const { return resource_id_; }

   private:
    String resource_id_;
  };

  static ResourcePoolLimiter& GetInstance();

  ~ResourcePoolLimiter();
  ResourcePoolLimiter(const ResourcePoolLimiter&) = delete;
  ResourcePoolLimiter& operator=(const ResourcePoolLimiter&) = delete;

  std::unique_ptr<ResourceInUseTracker> IssueResourceInUseTracker(
      ExecutionContext* context,
      ResourceType resource_type);

 private:
  ResourcePoolLimiter();

  void DropResourceInUse(const ResourceInUseTracker* resource_in_use_tracker);

  Mutex resources_in_use_lock_;
  HashMap<String, int> resources_in_use_;
};

}  // namespace blink

#endif  // BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_RESOURCE_POOL_LIMITER_RESOURCE_POOL_LIMITER_H_
