/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_SITE_VISIT_SITE_VISIT_OBSERVER_MOCK_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_SITE_VISIT_SITE_VISIT_OBSERVER_MOCK_H_

#include <cstdint>

#include "brave/components/brave_ads/core/internal/user_engagement/site_visit/site_visit_observer.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace brave_ads {

class SiteVisitObserverMock : public SiteVisitObserver {
 public:
  SiteVisitObserverMock();

  SiteVisitObserverMock(const SiteVisitObserverMock&) = delete;
  SiteVisitObserverMock& operator=(const SiteVisitObserverMock&) = delete;

  ~SiteVisitObserverMock() override;

  MOCK_METHOD(void, OnMaybeLandOnPage, (const AdInfo&, base::TimeDelta));
  MOCK_METHOD(void, OnDidSuspendPageLand, (int32_t, base::TimeDelta));
  MOCK_METHOD(void, OnDidResumePageLand, (int32_t, base::TimeDelta));
  MOCK_METHOD(void, OnDidLandOnPage, (int32_t, int, const AdInfo&));
  MOCK_METHOD(void, OnDidNotLandOnPage, (int32_t, const AdInfo&));
  MOCK_METHOD(void, OnCanceledPageLand, (int32_t, const AdInfo&));
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_SITE_VISIT_SITE_VISIT_OBSERVER_MOCK_H_
