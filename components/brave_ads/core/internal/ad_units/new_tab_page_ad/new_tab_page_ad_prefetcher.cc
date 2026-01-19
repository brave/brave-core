/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/ad_units/new_tab_page_ad/new_tab_page_ad_prefetcher.h"

#include <utility>

#include "base/check.h"
#include "brave/components/brave_ads/core/browser/service/ads_service.h"

namespace brave_ads {

NewTabPageAdPrefetcher::NewTabPageAdPrefetcher(AdsService& ads_service)
    : ads_service_(ads_service) {}

NewTabPageAdPrefetcher::~NewTabPageAdPrefetcher() = default;

mojom::NewTabPageAdInfoPtr NewTabPageAdPrefetcher::MaybeGetPrefetchedAd() {
  return std::move(mojom_prefetched_ad_);
}

void NewTabPageAdPrefetcher::Prefetch() {
  if (!mojom_prefetched_ad_ && !is_prefetching_) {
    is_prefetching_ = true;

    ads_service_->MaybeServeNewTabPageAd(
        base::BindOnce(&NewTabPageAdPrefetcher::PrefetchCallback,
                       weak_ptr_factory_.GetWeakPtr()));
  }
}

void NewTabPageAdPrefetcher::PrefetchCallback(
    mojom::NewTabPageAdInfoPtr mojom_ad) {
  CHECK(!mojom_prefetched_ad_);

  if (!is_prefetching_) {
    // `is_prefetching_` can be reset during shutdown, so fail gracefully.
    return;
  }
  is_prefetching_ = false;
  mojom_prefetched_ad_ = std::move(mojom_ad);
}

}  // namespace brave_ads
