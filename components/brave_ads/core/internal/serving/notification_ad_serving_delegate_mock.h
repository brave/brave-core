/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_NOTIFICATION_AD_SERVING_DELEGATE_MOCK_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_NOTIFICATION_AD_SERVING_DELEGATE_MOCK_H_

#include "brave/components/brave_ads/core/internal/serving/notification_ad_serving_delegate.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace brave_ads {

class NotificationAdServingDelegateMock : public NotificationAdServingDelegate {
 public:
  NotificationAdServingDelegateMock();

  NotificationAdServingDelegateMock(const NotificationAdServingDelegateMock&) =
      delete;
  NotificationAdServingDelegateMock& operator=(
      const NotificationAdServingDelegateMock&) = delete;

  ~NotificationAdServingDelegateMock() override;

  MOCK_METHOD(void,
              OnOpportunityAroseToServeNotificationAd,
              (const SegmentList& segments));

  MOCK_METHOD(void, OnDidServeNotificationAd, (const NotificationAdInfo& ad));

  MOCK_METHOD(void, OnFailedToServeNotificationAd, ());
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_NOTIFICATION_AD_SERVING_DELEGATE_MOCK_H_
