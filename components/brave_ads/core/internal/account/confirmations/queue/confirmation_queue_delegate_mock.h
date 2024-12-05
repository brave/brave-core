/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_QUEUE_CONFIRMATION_QUEUE_DELEGATE_MOCK_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_QUEUE_CONFIRMATION_QUEUE_DELEGATE_MOCK_H_

#include "brave/components/brave_ads/core/internal/account/confirmations/queue/confirmation_queue_delegate.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace brave_ads {

class ConfirmationQueueDelegateMock : public ConfirmationQueueDelegate {
 public:
  ConfirmationQueueDelegateMock();

  ConfirmationQueueDelegateMock(const ConfirmationQueueDelegateMock&) = delete;
  ConfirmationQueueDelegateMock& operator=(
      const ConfirmationQueueDelegateMock&) = delete;

  ~ConfirmationQueueDelegateMock() override;

  MOCK_METHOD(void,
              OnDidAddConfirmationToQueue,
              (const ConfirmationInfo& confirmation));

  MOCK_METHOD(void,
              OnFailedToAddConfirmationToQueue,
              (const ConfirmationInfo& confirmation));

  MOCK_METHOD(void,
              OnWillProcessConfirmationQueue,
              (const ConfirmationInfo& confirmation, base::Time process_at));

  MOCK_METHOD(void,
              OnDidProcessConfirmationQueue,
              (const ConfirmationInfo& confirmation));

  MOCK_METHOD(void,
              OnFailedToProcessConfirmationQueue,
              (const ConfirmationInfo& confirmation));

  MOCK_METHOD(void, OnFailedToProcessNextConfirmationInQueue, ());

  MOCK_METHOD(void, OnDidExhaustConfirmationQueue, ());
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_QUEUE_CONFIRMATION_QUEUE_DELEGATE_MOCK_H_
