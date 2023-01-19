/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_CONFIRMATIONS_CONFIRMATIONS_DELEGATE_MOCK_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_CONFIRMATIONS_CONFIRMATIONS_DELEGATE_MOCK_H_

#include "bat/ads/internal/account/confirmations/confirmations_delegate.h"
#include "testing/gmock/include/gmock/gmock.h"  // IWYU pragma: keep

namespace ads {

struct ConfirmationInfo;

class ConfirmationsDelegateMock : public ConfirmationsDelegate {
 public:
  ConfirmationsDelegateMock();

  ConfirmationsDelegateMock(const ConfirmationsDelegateMock& other) = delete;
  ConfirmationsDelegateMock& operator=(const ConfirmationsDelegateMock&) =
      delete;

  ConfirmationsDelegateMock(ConfirmationsDelegateMock&& other) noexcept =
      delete;
  ConfirmationsDelegateMock& operator=(
      ConfirmationsDelegateMock&& other) noexcept = delete;

  ~ConfirmationsDelegateMock() override;

  MOCK_METHOD(void, OnDidConfirm, (const ConfirmationInfo& confirmation));

  MOCK_METHOD(void, OnFailedToConfirm, (const ConfirmationInfo& confirmation));
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_CONFIRMATIONS_CONFIRMATIONS_DELEGATE_MOCK_H_
