/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DEPRECATED_USER_ENGAGEMENT_CONVERSIONS_QUEUE_CONVERSION_QUEUE_DELEGATE_MOCK_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DEPRECATED_USER_ENGAGEMENT_CONVERSIONS_QUEUE_CONVERSION_QUEUE_DELEGATE_MOCK_H_

#include "brave/components/brave_ads/core/internal/deprecated/user_engagement/conversions/queue/conversion_queue_delegate.h"
#include "testing/gmock/include/gmock/gmock.h"  // IWYU pragma: keep

namespace brave_ads {

class ConversionQueueDelegateMock : public ConversionQueueDelegate {
 public:
  ConversionQueueDelegateMock();

  ConversionQueueDelegateMock(const ConversionQueueDelegateMock&) = delete;
  ConversionQueueDelegateMock& operator=(const ConversionQueueDelegateMock&) =
      delete;

  ConversionQueueDelegateMock(ConversionQueueDelegateMock&&) noexcept = delete;
  ConversionQueueDelegateMock& operator=(
      ConversionQueueDelegateMock&&) noexcept = delete;

  ~ConversionQueueDelegateMock() override;

  MOCK_METHOD(void,
              OnDidAddConversionToQueue,
              (const ConversionInfo& conversion));

  MOCK_METHOD(void,
              OnFailedToAddConversionToQueue,
              (const ConversionInfo& conversion));

  MOCK_METHOD(void,
              OnWillProcessConversionQueue,
              (const ConversionInfo& conversion, base::Time process_at));

  MOCK_METHOD(void,
              OnDidProcessConversionQueue,
              (const ConversionInfo& conversion));

  MOCK_METHOD(void,
              OnFailedToProcessConversionQueue,
              (const ConversionInfo& conversion));

  MOCK_METHOD(void, OnFailedToProcessNextConversionInQueue, ());

  MOCK_METHOD(void, OnDidExhaustConversionQueue, ());
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DEPRECATED_USER_ENGAGEMENT_CONVERSIONS_QUEUE_CONVERSION_QUEUE_DELEGATE_MOCK_H_
