/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/subdivision/test/test_subdivision_observer.h"

// Allows tests to synchronously wait for `Subdivision` to notify
// `OnDidUpdateSubdivision` without polling or manual futures.

namespace brave_ads {

TestSubdivisionObserver::TestSubdivisionObserver(
    Subdivision* subdivision,
    std::string expected_subdivision)
    : expected_subdivision_(std::move(expected_subdivision)) {
  scoped_observation_.Observe(subdivision);
}

TestSubdivisionObserver::~TestSubdivisionObserver() = default;

bool TestSubdivisionObserver::WaitForDidUpdateSubdivision() {
  return did_update_subdivision_future_.Wait();
}

void TestSubdivisionObserver::OnDidUpdateSubdivision(
    const std::string& subdivision) {
  if (subdivision == expected_subdivision_) {
    did_update_subdivision_future_.GetCallback().Run();
  }
}

}  // namespace brave_ads
