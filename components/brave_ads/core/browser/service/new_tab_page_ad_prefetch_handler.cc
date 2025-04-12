/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/browser/service/new_tab_page_ad_prefetch_handler.h"

#include "base/check.h"
#include "brave/components/brave_ads/core/browser/service/ads_service.h"
#include "brave/components/brave_ads/core/public/ad_units/new_tab_page_ad/new_tab_page_ad_info.h"

namespace brave_ads {

NewTabPageAdPrefetchHandler::NewTabPageAdPrefetchHandler(
    AdsService& ads_service)
    : ads_service_(ads_service) {}

NewTabPageAdPrefetchHandler::~NewTabPageAdPrefetchHandler() = default;

std::optional<NewTabPageAdInfo>
NewTabPageAdPrefetchHandler::MaybeGetPrefetchedNewTabPageAd() {
  std::optional<NewTabPageAdInfo> ad;
  if (prefetched_new_tab_page_ad_ && prefetched_new_tab_page_ad_->IsValid()) {
    ad = prefetched_new_tab_page_ad_;
  }
  prefetched_new_tab_page_ad_.reset();

  return ad;
}

void NewTabPageAdPrefetchHandler::PrefetchNewTabPageAd() {
  if (!prefetched_new_tab_page_ad_ && !is_prefetching_new_tab_page_ad_) {
    is_prefetching_new_tab_page_ad_ = true;

    ads_service_->MaybeServeNewTabPageAd(base::BindOnce(
        &NewTabPageAdPrefetchHandler::PrefetchNewTabPageAdCallback,
        weak_ptr_factory_.GetWeakPtr()));
  }
}

void NewTabPageAdPrefetchHandler::PrefetchNewTabPageAdCallback(
    base::optional_ref<const NewTabPageAdInfo> ad) {
  CHECK(!prefetched_new_tab_page_ad_);

  if (!is_prefetching_new_tab_page_ad_) {
    // `is_prefetching_new_tab_page_ad_` can be reset during shutdown, so fail
    // gracefully.
    return;
  }
  is_prefetching_new_tab_page_ad_ = false;

  if (ad) {
    prefetched_new_tab_page_ad_ = *ad;
  }
}

}  // namespace brave_ads
