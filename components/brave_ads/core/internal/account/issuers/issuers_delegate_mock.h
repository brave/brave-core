/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_ISSUERS_ISSUERS_DELEGATE_MOCK_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_ISSUERS_ISSUERS_DELEGATE_MOCK_H_

#include "brave/components/brave_ads/core/internal/account/issuers/issuers_delegate.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace brave_ads {

class IssuersDelegateMock : public IssuersDelegate {
 public:
  IssuersDelegateMock();

  IssuersDelegateMock(const IssuersDelegateMock&) = delete;
  IssuersDelegateMock& operator=(const IssuersDelegateMock&) = delete;

  ~IssuersDelegateMock() override;

  MOCK_METHOD(void, OnWillFetchIssuers, (base::Time));
  MOCK_METHOD(void, OnDidFetchIssuers, (const IssuersInfo&));
  MOCK_METHOD(void, OnFailedToFetchIssuers, ());
  MOCK_METHOD(void, OnWillRetryFetchingIssuers, (base::Time));
  MOCK_METHOD(void, OnDidRetryFetchingIssuers, ());
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_ISSUERS_ISSUERS_DELEGATE_MOCK_H_
