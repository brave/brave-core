/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_ISSUERS_ISSUERS_DELEGATE_MOCK_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_ISSUERS_ISSUERS_DELEGATE_MOCK_H_

#include "bat/ads/internal/account/issuers/issuers_delegate.h"
#include "testing/gmock/include/gmock/gmock.h"  // IWYU pragma: keep

namespace ads {

struct IssuersInfo;

class IssuersDelegateMock : public IssuersDelegate {
 public:
  IssuersDelegateMock();

  IssuersDelegateMock(const IssuersDelegateMock& other) = delete;
  IssuersDelegateMock& operator=(const IssuersDelegateMock& other) = delete;

  IssuersDelegateMock(IssuersDelegateMock&& other) noexcept = delete;
  IssuersDelegateMock& operator=(IssuersDelegateMock&& other) noexcept = delete;

  ~IssuersDelegateMock() override;

  MOCK_METHOD(void, OnDidFetchIssuers, (const IssuersInfo& issuers));

  MOCK_METHOD(void, OnFailedToFetchIssuers, ());

  MOCK_METHOD(void, OnWillRetryFetchingIssuers, (const base::Time retry_at));

  MOCK_METHOD(void, OnDidRetryFetchingIssuers, ());
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_ISSUERS_ISSUERS_DELEGATE_MOCK_H_
