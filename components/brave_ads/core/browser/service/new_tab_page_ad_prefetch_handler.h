/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_BROWSER_SERVICE_NEW_TAB_PAGE_AD_PREFETCH_HANDLER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_BROWSER_SERVICE_NEW_TAB_PAGE_AD_PREFETCH_HANDLER_H_

#include <optional>

#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "base/types/optional_ref.h"
#include "brave/components/brave_ads/core/public/ad_units/new_tab_page_ad/new_tab_page_ad_info.h"

namespace brave_ads {

class AdsService;

class NewTabPageAdPrefetchHandler {
 public:
  explicit NewTabPageAdPrefetchHandler(AdsService& ads_service);
  NewTabPageAdPrefetchHandler(const NewTabPageAdPrefetchHandler&) = delete;
  NewTabPageAdPrefetchHandler& operator=(const NewTabPageAdPrefetchHandler&) =
      delete;
  ~NewTabPageAdPrefetchHandler();

  void PrefetchNewTabPageAd();
  std::optional<NewTabPageAdInfo> MaybeGetPrefetchedNewTabPageAd();

 private:
  void PrefetchNewTabPageAdCallback(
      base::optional_ref<const NewTabPageAdInfo> ad);

  std::optional<NewTabPageAdInfo> prefetched_new_tab_page_ad_;
  bool is_prefetching_new_tab_page_ad_ = false;

  const raw_ref<AdsService> ads_service_;

  base::WeakPtrFactory<NewTabPageAdPrefetchHandler> weak_ptr_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_BROWSER_SERVICE_NEW_TAB_PAGE_AD_PREFETCH_HANDLER_H_
