/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CATALOG_CATALOG_REQUEST_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CATALOG_CATALOG_REQUEST_H_

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/common/interfaces/brave_ads.mojom-forward.h"
#include "brave/components/brave_ads/core/ads_client_notifier_observer.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_request_delegate.h"
#include "brave/components/brave_ads/core/internal/common/timer/backoff_timer.h"
#include "brave/components/brave_ads/core/internal/common/timer/timer.h"
#include "brave/components/brave_ads/core/internal/database/database_manager_observer.h"

namespace brave_ads {

class CatalogRequest final : public AdsClientNotifierObserver,
                             public DatabaseManagerObserver {
 public:
  CatalogRequest();

  CatalogRequest(const CatalogRequest&) = delete;
  CatalogRequest& operator=(const CatalogRequest&) = delete;

  CatalogRequest(CatalogRequest&&) noexcept = delete;
  CatalogRequest& operator=(CatalogRequest&&) noexcept = delete;

  ~CatalogRequest() override;

  void SetDelegate(CatalogRequestDelegate* delegate) {
    CHECK_EQ(delegate_, nullptr);
    delegate_ = delegate;
  }

  void PeriodicallyFetch();

 private:
  void Fetch();
  void FetchCallback(const mojom::UrlResponseInfo& url_response);

  void FetchAfterDelay();

  void Retry();
  void RetryCallback();
  void StopRetrying();

  raw_ptr<CatalogRequestDelegate> delegate_ = nullptr;

  bool is_periodically_fetching_ = false;

  bool is_fetching_ = false;

  Timer timer_;
  BackoffTimer retry_timer_;

  base::WeakPtrFactory<CatalogRequest> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CATALOG_CATALOG_REQUEST_H_
