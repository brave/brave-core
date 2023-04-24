/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_INLINE_CONTENT_AD_SERVING_DELEGATE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_INLINE_CONTENT_AD_SERVING_DELEGATE_H_

#include "brave/components/brave_ads/core/internal/segments/segment_alias.h"

namespace brave_ads {

struct InlineContentAdInfo;

class InlineContentAdServingDelegate {
 public:
  // Invoked when an opportunity arises to serve an inline content ad for the
  // |segments|.
  virtual void OnOpportunityAroseToServeInlineContentAd(
      const SegmentList& segments) {}

  // Invoked when an inline content ad is served.
  virtual void OnDidServeInlineContentAd(const InlineContentAdInfo& ad) {}

  // Invoked when an inline content ad fails to serve.
  virtual void OnFailedToServeInlineContentAd() {}

 protected:
  virtual ~InlineContentAdServingDelegate() = default;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_INLINE_CONTENT_AD_SERVING_DELEGATE_H_
