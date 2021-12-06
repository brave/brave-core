/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_TOKENS_ISSUERS_ISSUERS_DELEGATE_MOCK_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_TOKENS_ISSUERS_ISSUERS_DELEGATE_MOCK_H_

#include "bat/ads/internal/tokens/issuers/issuers_delegate.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace ads {

struct IssuersInfo;

class IssuersDelegateMock : public IssuersDelegate {
 public:
  IssuersDelegateMock();
  ~IssuersDelegateMock() override;

  IssuersDelegateMock(const IssuersDelegateMock&) = delete;
  IssuersDelegateMock& operator=(const IssuersDelegateMock&) = delete;

  MOCK_METHOD(void, OnDidGetIssuers, (const IssuersInfo& issuers));

  MOCK_METHOD(void, OnFailedToGetIssuers, ());
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_TOKENS_ISSUERS_ISSUERS_DELEGATE_MOCK_H_
