/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_INLINE_CONTENT_AD_SERVING_OBSERVER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_INLINE_CONTENT_AD_SERVING_OBSERVER_H_

#include "base/observer_list_types.h"
#include "bat/ads/internal/segments/segment_alias.h"

namespace ads {

struct InlineContentAdInfo;

namespace inline_content_ads {

class ServingObserver : public base::CheckedObserver {
 public:
  // Invoked when an opportunity arises to serve an inline content ad for the
  // |segments|.
  virtual void OnOpportunityAroseToServeInlineContentAd(
      const SegmentList& segments) {}

  // Invoked when an inline content ad is served.
  virtual void OnDidServeInlineContentAd(const InlineContentAdInfo& ad) {}

  // Invoked when an inline content ad fails to serve.
  virtual void OnFailedToServeInlineContentAd() {}
};

}  // namespace inline_content_ads
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_INLINE_CONTENT_AD_SERVING_OBSERVER_H_
