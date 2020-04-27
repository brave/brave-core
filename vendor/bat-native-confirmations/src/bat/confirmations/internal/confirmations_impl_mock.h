/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_CONFIRMATIONS_INTERNAL_CONFIRMATIONS_IMPL_MOCK_H_
#define BAT_CONFIRMATIONS_INTERNAL_CONFIRMATIONS_IMPL_MOCK_H_

#include "testing/gmock/include/gmock/gmock.h"

#include "bat/confirmations/internal/confirmations_impl.h"

namespace confirmations {

class ConfirmationsImplMock : public ConfirmationsImpl {
 public:
  explicit ConfirmationsImplMock(
      ConfirmationsClient* confirmations_client);

  ~ConfirmationsImplMock() override;

  MOCK_METHOD0(SaveState, void());
};

}  // namespace confirmations

#endif  // BAT_CONFIRMATIONS_INTERNAL_CONFIRMATIONS_IMPL_MOCK_H_
