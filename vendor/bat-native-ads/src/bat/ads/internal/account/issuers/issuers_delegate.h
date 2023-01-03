/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_ISSUERS_ISSUERS_DELEGATE_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_ISSUERS_ISSUERS_DELEGATE_H_

#include "base/time/time.h"

namespace ads {

struct IssuersInfo;

class IssuersDelegate {
 public:
  // Invoked to tell the delegate we successfully fetched the |issuers|.
  virtual void OnDidFetchIssuers(const IssuersInfo& issuers) {}

  // Invoked to tell the delegate we failed to fetch the issuers.
  virtual void OnFailedToFetchIssuers() {}

  // Invoked to tell the delegate we will retry fetching the issuers at
  // |retry_at|.
  virtual void OnWillRetryFetchingIssuers(const base::Time retry_at) {}

  // Invoked to tell the delegate we retried fetching the issuers.
  virtual void OnDidRetryFetchingIssuers() {}

 protected:
  virtual ~IssuersDelegate() = default;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_ISSUERS_ISSUERS_DELEGATE_H_
