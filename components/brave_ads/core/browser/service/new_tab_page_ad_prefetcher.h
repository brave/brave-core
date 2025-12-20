/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_BROWSER_SERVICE_NEW_TAB_PAGE_AD_PREFETCHER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_BROWSER_SERVICE_NEW_TAB_PAGE_AD_PREFETCHER_H_

#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads {

class AdsService;

class NewTabPageAdPrefetcher {
 public:
  explicit NewTabPageAdPrefetcher(AdsService& ads_service);

  NewTabPageAdPrefetcher(const NewTabPageAdPrefetcher&) = delete;
  NewTabPageAdPrefetcher& operator=(const NewTabPageAdPrefetcher&) = delete;

  ~NewTabPageAdPrefetcher();

  void Prefetch();
  mojom::NewTabPageAdInfoPtr MaybeGetPrefetchedAd();

 private:
  void PrefetchCallback(mojom::NewTabPageAdInfoPtr mojom_ad);

  mojom::NewTabPageAdInfoPtr mojom_prefetched_ad_;
  bool is_prefetching_ = false;

  const raw_ref<AdsService> ads_service_;

  base::WeakPtrFactory<NewTabPageAdPrefetcher> weak_ptr_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_BROWSER_SERVICE_NEW_TAB_PAGE_AD_PREFETCHER_H_
