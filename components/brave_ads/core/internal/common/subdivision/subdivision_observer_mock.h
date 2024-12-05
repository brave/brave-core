/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_SUBDIVISION_SUBDIVISION_OBSERVER_MOCK_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_SUBDIVISION_SUBDIVISION_OBSERVER_MOCK_H_

#include <string>

#include "brave/components/brave_ads/core/internal/common/subdivision/subdivision_observer.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace brave_ads {

class SubdivisionObserverMock : public SubdivisionObserver {
 public:
  SubdivisionObserverMock();

  SubdivisionObserverMock(const SubdivisionObserverMock&) = delete;
  SubdivisionObserverMock& operator=(const SubdivisionObserverMock&) = delete;

  ~SubdivisionObserverMock() override;

  MOCK_METHOD(void, OnDidUpdateSubdivision, (const std::string& subdivision));
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_SUBDIVISION_SUBDIVISION_OBSERVER_MOCK_H_
