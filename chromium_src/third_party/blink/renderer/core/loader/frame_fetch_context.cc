/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "src/third_party/blink/renderer/core/loader/frame_fetch_context.cc"

namespace blink {

String FrameFetchContext::GetCacheIdentifierIfCrossSiteSubframe() const {
  if (GetResourceFetcherProperties().IsDetached())
    return cache_identifier_if_cross_site_subframe_;

  // Make sure to always use the actual data, because the frame can be reused
  // and identifiers can be changed.
  String cache_identifier;
  if (document_->domWindow()->IsCrossSiteSubframeIncludingScheme()) {
    if (auto top_frame_origin = GetTopFrameOrigin()) {
      if (top_frame_origin_for_cache_identifier_ == top_frame_origin) {
        return cache_identifier_if_cross_site_subframe_;
      } else {
        // Cache top frame origin to skip unnecessary RegistrableDomain/Host
        // calls when nothing has changed.
        top_frame_origin_for_cache_identifier_ = top_frame_origin;
        cache_identifier = top_frame_origin->RegistrableDomain();
        if (cache_identifier.IsEmpty())
          cache_identifier = top_frame_origin->Host();
      }
    }
  }

  cache_identifier_if_cross_site_subframe_ = cache_identifier;
  return cache_identifier_if_cross_site_subframe_;
}
}  // namespace blink
