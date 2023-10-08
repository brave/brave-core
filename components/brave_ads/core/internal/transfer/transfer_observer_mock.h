/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TRANSFER_TRANSFER_OBSERVER_MOCK_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TRANSFER_TRANSFER_OBSERVER_MOCK_H_

#include <cstdint>

#include "brave/components/brave_ads/core/internal/transfer/transfer_observer.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace brave_ads {

class TransferObserverMock : public TransferObserver {
 public:
  TransferObserverMock();

  TransferObserverMock(const TransferObserverMock&) = delete;
  TransferObserverMock& operator=(const TransferObserverMock&) = delete;

  TransferObserverMock(TransferObserverMock&&) noexcept = delete;
  TransferObserverMock& operator=(TransferObserverMock&&) noexcept = delete;

  ~TransferObserverMock() override;

  MOCK_METHOD(void,
              OnWillTransferAd,
              (const AdInfo& ad, const base::Time time));

  MOCK_METHOD(void, OnDidTransferAd, (const AdInfo& ad));

  MOCK_METHOD(void,
              OnCanceledTransfer,
              (const AdInfo& ad, const int32_t tab_id));

  MOCK_METHOD(void, OnFailedToTransferAd, (const AdInfo& ad));
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TRANSFER_TRANSFER_OBSERVER_MOCK_H_
