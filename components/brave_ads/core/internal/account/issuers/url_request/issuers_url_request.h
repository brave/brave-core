/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_ISSUERS_URL_REQUEST_ISSUERS_URL_REQUEST_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_ISSUERS_URL_REQUEST_ISSUERS_URL_REQUEST_H_

#include "base/check_op.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/core/internal/account/issuers/url_request/issuers_url_request_delegate.h"
#include "brave/components/brave_ads/core/internal/common/timer/backoff_timer.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"

namespace brave_ads {

struct IssuersInfo;

class IssuersUrlRequest final {
 public:
  IssuersUrlRequest();

  IssuersUrlRequest(const IssuersUrlRequest&) = delete;
  IssuersUrlRequest& operator=(const IssuersUrlRequest&) = delete;

  IssuersUrlRequest(IssuersUrlRequest&&) noexcept = delete;
  IssuersUrlRequest& operator=(IssuersUrlRequest&&) noexcept = delete;

  ~IssuersUrlRequest();

  void SetDelegate(IssuersUrlRequestDelegate* delegate) {
    CHECK_EQ(delegate_, nullptr);
    delegate_ = delegate;
  }

  void PeriodicallyFetch();

 private:
  void Fetch();
  void FetchCallback(const mojom::UrlResponseInfo& mojom_url_response);
  void FetchAfterDelay();

  void SuccessfullyFetchedIssuers(const IssuersInfo& issuers);
  void FailedToFetchIssuers();

  void Retry();
  void RetryCallback();
  void StopRetrying();

  void NotifyDidFetchIssuers(const IssuersInfo& issuers) const;
  void NotifyFailedToFetchIssuers() const;
  void NotifyWillFetchIssuers(base::Time fetch_at) const;
  void NotifyWillRetryFetchingIssuers(base::Time retry_at) const;
  void NotifyDidRetryFetchingIssuers() const;

  raw_ptr<IssuersUrlRequestDelegate> delegate_ = nullptr;  // Not owned.

  bool is_periodically_fetching_ = false;

  bool is_fetching_ = false;

  BackoffTimer timer_;

  base::WeakPtrFactory<IssuersUrlRequest> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_ISSUERS_URL_REQUEST_ISSUERS_URL_REQUEST_H_
