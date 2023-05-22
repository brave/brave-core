/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_ISSUERS_ISSUERS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_ISSUERS_ISSUERS_H_

#include <string>

#include "base/check_op.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/common/interfaces/brave_ads.mojom-forward.h"
#include "brave/components/brave_ads/core/ads_client_notifier_observer.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_delegate.h"
#include "brave/components/brave_ads/core/internal/common/timer/backoff_timer.h"
#include "brave/components/brave_ads/core/internal/common/timer/timer.h"

namespace brave_ads {

struct IssuersInfo;

class Issuers final : public AdsClientNotifierObserver {
 public:
  Issuers();

  Issuers(const Issuers&) = delete;
  Issuers& operator=(const Issuers&) = delete;

  Issuers(Issuers&&) noexcept = delete;
  Issuers& operator=(Issuers&&) noexcept = delete;

  ~Issuers() override;

  void SetDelegate(IssuersDelegate* delegate) {
    CHECK_EQ(delegate_, nullptr);
    delegate_ = delegate;
  }

  void MaybeFetch();

 private:
  void MaybeFetchOrReset();

  void Fetch();
  void FetchCallback(const mojom::UrlResponseInfo& url_response);

  void Reset();

  void SuccessfullyFetchedIssuers(const IssuersInfo& issuers);
  void FailedToFetchIssuers(bool should_retry);

  void FetchAfterDelay();
  void CancelFetch();

  void Retry();
  void RetryCallback();
  void StopRetrying();

  // AdsClientNotifierObserver:
  void OnNotifyPrefDidChange(const std::string& path) override;

  raw_ptr<IssuersDelegate> delegate_ = nullptr;

  bool is_fetching_ = false;
  bool did_fetch_ = false;

  Timer timer_;
  BackoffTimer retry_timer_;

  base::WeakPtrFactory<Issuers> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_ISSUERS_ISSUERS_H_
