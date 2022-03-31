/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_CONFIRMATIONS_CONFIRMATIONS_DELEGATE_MOCK_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_CONFIRMATIONS_CONFIRMATIONS_DELEGATE_MOCK_H_

#include "bat/ads/internal/account/confirmations/confirmations_delegate.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace ads {

struct ConfirmationInfo;

class ConfirmationsDelegateMock : public ConfirmationsDelegate {
 public:
  ConfirmationsDelegateMock();
  ~ConfirmationsDelegateMock() override;

  ConfirmationsDelegateMock(const ConfirmationsDelegateMock&) = delete;
  ConfirmationsDelegateMock& operator=(const ConfirmationsDelegateMock&) =
      delete;

  MOCK_METHOD(void, OnDidConfirm, (const ConfirmationInfo& confirmation));

  MOCK_METHOD(void, OnFailedToConfirm, (const ConfirmationInfo& confirmation));
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_CONFIRMATIONS_CONFIRMATIONS_DELEGATE_MOCK_H_
