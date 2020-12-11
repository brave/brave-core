/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_RPILL_RPILL_HELPER_MOCK_H_
#define BAT_ADS_INTERNAL_RPILL_RPILL_HELPER_MOCK_H_

#include "testing/gmock/include/gmock/gmock.h"
#include "bat/ads/internal/rpill/rpill_helper.h"

namespace ads {

class RPillHelperMock : public RPillHelper {
 public:
  RPillHelperMock();
  ~RPillHelperMock() override;

  RPillHelperMock(const RPillHelperMock&) = delete;
  RPillHelperMock& operator=(const RPillHelperMock&) = delete;

  MOCK_CONST_METHOD0(IsUncertainFuture, bool());
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_RPILL_RPILL_HELPER_MOCK_H_
