/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_CONVERSIONS_CONVERSIONS_OBSERVER_MOCK_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_CONVERSIONS_CONVERSIONS_OBSERVER_MOCK_H_

#include <string>

#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversions_observer.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace brave_ads {

class ConversionsObserverMock : public ConversionsObserver {
 public:
  ConversionsObserverMock();

  ConversionsObserverMock(const ConversionsObserverMock&) = delete;
  ConversionsObserverMock& operator=(const ConversionsObserverMock&) = delete;

  ~ConversionsObserverMock() override;

  MOCK_METHOD(void, OnDidConvertAd, (const ConversionInfo& conversion));

  MOCK_METHOD(void,
              OnFailedToConvertAd,
              (const std::string& creative_instance_id));
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_CONVERSIONS_CONVERSIONS_OBSERVER_MOCK_H_
