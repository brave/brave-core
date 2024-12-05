/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_CONFIRMATIONS_DELEGATE_MOCK_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_CONFIRMATIONS_DELEGATE_MOCK_H_

#include "brave/components/brave_ads/core/internal/account/confirmations/confirmations_delegate.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace brave_ads {

class ConfirmationsDelegateMock : public ConfirmationDelegate {
 public:
  ConfirmationsDelegateMock();

  ConfirmationsDelegateMock(const ConfirmationsDelegateMock&) = delete;
  ConfirmationsDelegateMock& operator=(const ConfirmationsDelegateMock&) =
      delete;

  ~ConfirmationsDelegateMock() override;

  MOCK_METHOD(void, OnDidConfirm, (const ConfirmationInfo& confirmation));

  MOCK_METHOD(void, OnFailedToConfirm, (const ConfirmationInfo& confirmation));
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_CONFIRMATIONS_DELEGATE_MOCK_H_
