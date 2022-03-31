/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/platform/loader/fetch/resource_fetcher.h"

#include "third_party/blink/public/common/features.h"
#include "third_party/blink/renderer/platform/loader/fetch/memory_cache.h"

// Replace DefaultCacheIdentifier call with our method using this macro.
#define DefaultCacheIdentifier                                     \
  DefaultCacheIdentifier().IsEmpty() ? GetContextCacheIdentifier() \
                                     : GetContextCacheIdentifier

#include "src/third_party/blink/renderer/platform/loader/fetch/resource_fetcher.cc"

#undef DefaultCacheIdentifier

namespace blink {

// This method returns a custom cache identifier for a Context to be used in
// blink::MemoryCache to properly partition requests from third-party frames
// when already existing entries in blink::MemoryCache should not be used.
String ResourceFetcher::GetContextCacheIdentifier() const {
  if (!base::FeatureList::IsEnabled(features::kPartitionBlinkMemoryCache)) {
    return MemoryCache::DefaultCacheIdentifier();
  }
  if (!properties_->IsMainFrame()) {
    if (auto cache_identifier =
            Context().GetCacheIdentifierIfCrossSiteSubframe()) {
      return cache_identifier;
    }
  }
  return MemoryCache::DefaultCacheIdentifier();
}

}  // namespace blink
