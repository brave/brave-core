/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_SITE_VISIT_SITE_VISIT_OBSERVER_MOCK_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_SITE_VISIT_SITE_VISIT_OBSERVER_MOCK_H_

#include <cstdint>

#include "brave/components/brave_ads/core/internal/tabs/tab_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/site_visit/site_visit_observer.h"
#include "testing/gmock/include/gmock/gmock.h"  // IWYU pragma: keep

namespace brave_ads {

class SiteVisitObserverMock : public SiteVisitObserver {
 public:
  SiteVisitObserverMock();

  SiteVisitObserverMock(const SiteVisitObserverMock&) = delete;
  SiteVisitObserverMock& operator=(const SiteVisitObserverMock&) = delete;

  SiteVisitObserverMock(SiteVisitObserverMock&&) noexcept = delete;
  SiteVisitObserverMock& operator=(SiteVisitObserverMock&&) noexcept = delete;

  ~SiteVisitObserverMock() override;

  MOCK_METHOD(void,
              OnMaybeLandOnPage,
              (const AdInfo& ad, const base::Time maybe_at));

  MOCK_METHOD(void, OnDidLandOnPage, (const TabInfo& tab, const AdInfo& ad));

  MOCK_METHOD(void, OnDidNotLandOnPage, (const AdInfo& ad));

  MOCK_METHOD(void,
              OnCanceledPageLand,
              (const AdInfo& ad, const int32_t tab_id));
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_SITE_VISIT_SITE_VISIT_OBSERVER_MOCK_H_
