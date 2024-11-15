/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CATALOG_CATALOG_URL_REQUEST_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CATALOG_CATALOG_URL_REQUEST_H_

#include "base/check_op.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_url_request_delegate.h"
#include "brave/components/brave_ads/core/internal/common/timer/backoff_timer.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"

namespace brave_ads {

struct CatalogInfo;

class CatalogUrlRequest final {
 public:
  CatalogUrlRequest();

  CatalogUrlRequest(const CatalogUrlRequest&) = delete;
  CatalogUrlRequest& operator=(const CatalogUrlRequest&) = delete;

  CatalogUrlRequest(CatalogUrlRequest&&) noexcept = delete;
  CatalogUrlRequest& operator=(CatalogUrlRequest&&) noexcept = delete;

  ~CatalogUrlRequest();

  void SetDelegate(CatalogUrlRequestDelegate* delegate) {
    CHECK_EQ(delegate_, nullptr);
    delegate_ = delegate;
  }

  void PeriodicallyFetch();

 private:
  void Fetch();
  void FetchCallback(const mojom::UrlResponseInfo& mojom_url_response);
  void FetchAfterDelay();

  void SuccessfullyFetchedCatalog(const CatalogInfo& catalog);
  void FailedToFetchCatalog();

  void Retry();
  void RetryCallback();
  void StopRetrying();

  void NotifyWillFetchCatalog(base::Time fetch_at) const;
  void NotifyDidFetchCatalog(const CatalogInfo& catalog) const;
  void NotifyFailedToFetchCatalog() const;
  void NotifyWillRetryFetchingCatalog(base::Time retry_at) const;
  void NotifyDidRetryFetchingCatalog() const;

  raw_ptr<CatalogUrlRequestDelegate> delegate_ = nullptr;  // Not owned.

  bool is_periodically_fetching_ = false;

  bool is_fetching_ = false;

  BackoffTimer timer_;

  base::WeakPtrFactory<CatalogUrlRequest> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CATALOG_CATALOG_URL_REQUEST_H_
