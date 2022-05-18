/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_ISSUERS_ISSUERS_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_ISSUERS_ISSUERS_H_

#include "base/check_op.h"
#include "base/memory/raw_ptr.h"
#include "bat/ads/internal/account/issuers/issuers_delegate.h"
#include "bat/ads/internal/base/backoff_timer.h"
#include "bat/ads/internal/base/timer.h"
#include "bat/ads/public/interfaces/ads.mojom.h"

namespace ads {

struct IssuersInfo;

class Issuers {
 public:
  Issuers();
  ~Issuers();

  void set_delegate(IssuersDelegate* delegate) {
    DCHECK_EQ(delegate_, nullptr);
    delegate_ = delegate;
  }

  void MaybeFetch();

 private:
  void Fetch();
  void OnFetch(const mojom::UrlResponse& url_response);

  void OnDidGetIssuers(const IssuersInfo& issuers);
  void OnFailedToGetIssuers();

  void FetchAfterDelay();
  base::TimeDelta GetFetchDelay() const;

  void Retry();
  void OnRetry();
  void StopRetrying();

  raw_ptr<IssuersDelegate> delegate_ = nullptr;

  bool is_fetching_ = false;

  Timer timer_;
  BackoffTimer retry_timer_;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_ISSUERS_ISSUERS_H_
