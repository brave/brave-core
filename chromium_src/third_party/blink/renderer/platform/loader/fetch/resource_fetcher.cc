/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/platform/loader/fetch/resource_fetcher.h"

#include "third_party/blink/public/common/features.h"
#include "third_party/blink/renderer/platform/loader/fetch/memory_cache.h"

#define DefaultCacheIdentifier                           \
  DefaultCacheIdentifier().IsEmpty()                     \
      ? GetCacheIdentifierIfCrossSiteSubframeOrDefault() \
      : GetCacheIdentifierIfCrossSiteSubframeOrDefault

#include "../../../../../../../../third_party/blink/renderer/platform/loader/fetch/resource_fetcher.cc"

#undef DefaultCacheIdentifier

namespace blink {

String ResourceFetcher::GetCacheIdentifierIfCrossSiteSubframeOrDefault() const {
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
