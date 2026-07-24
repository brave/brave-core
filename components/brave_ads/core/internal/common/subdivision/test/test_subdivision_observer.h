/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_SUBDIVISION_TEST_TEST_SUBDIVISION_OBSERVER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_SUBDIVISION_TEST_TEST_SUBDIVISION_OBSERVER_H_

#include <string>

#include "base/scoped_observation.h"
#include "base/test/test_future.h"
#include "brave/components/brave_ads/core/internal/common/subdivision/subdivision.h"
#include "brave/components/brave_ads/core/internal/common/subdivision/subdivision_observer.h"

namespace brave_ads {

class TestSubdivisionObserver final : public SubdivisionObserver {
 public:
  TestSubdivisionObserver(Subdivision* subdivision,
                          std::string expected_subdivision);

  TestSubdivisionObserver(const TestSubdivisionObserver&) = delete;
  TestSubdivisionObserver& operator=(const TestSubdivisionObserver&) = delete;

  ~TestSubdivisionObserver() override;

  bool WaitForDidUpdateSubdivision();

 private:
  // SubdivisionObserver:
  void OnDidUpdateSubdivision(const std::string& subdivision) override;

  const std::string expected_subdivision_;
  base::test::TestFuture<void> did_update_subdivision_future_;
  base::ScopedObservation<Subdivision, SubdivisionObserver> scoped_observation_{
      this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_SUBDIVISION_TEST_TEST_SUBDIVISION_OBSERVER_H_
