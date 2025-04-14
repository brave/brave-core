/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_REACTIONS_REACTIONS_OBSERVER_MOCK_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_REACTIONS_REACTIONS_OBSERVER_MOCK_H_

#include <string>

#include "brave/components/brave_ads/core/internal/user_engagement/reactions/reactions_observer.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace brave_ads {

class ReactionsObserverMock : public ReactionsObserver {
 public:
  ReactionsObserverMock();

  ReactionsObserverMock(const ReactionsObserverMock&) = delete;
  ReactionsObserverMock& operator=(const ReactionsObserverMock&) = delete;

  ~ReactionsObserverMock() override;

  MOCK_METHOD(void, OnDidLikeAd, (const std::string&));
  MOCK_METHOD(void, OnDidDislikeAd, (const std::string&));

  MOCK_METHOD(void, OnDidLikeSegment, (const std::string&));
  MOCK_METHOD(void, OnDidDislikeSegment, (const std::string&));

  MOCK_METHOD(void, OnDidToggleSaveAd, (const std::string&));

  MOCK_METHOD(void, OnDidToggleMarkAdAsInappropriate, (const std::string&));
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_REACTIONS_REACTIONS_OBSERVER_MOCK_H_
