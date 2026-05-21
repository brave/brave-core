// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_BRAVE_SHIELDS_AD_BLOCK_BROWSER_TEST_HELPER_H_
#define BRAVE_BROWSER_BRAVE_SHIELDS_AD_BLOCK_BROWSER_TEST_HELPER_H_

#include <memory>

#include "base/callback_list.h"
#include "base/functional/callback_forward.h"
#include "base/functional/callback_helpers.h"

namespace content {
class BrowserContext;
}

namespace brave_shields {

class AdBlockServiceTestObserver;

// Runs the ad block service's task runner until idle.
bool WaitForAdBlockServiceThreads();

// Ensures the ad block service is created during browser context setup and
// that an empty filter list catalog is delivered so gates are unblocked.
// Accepts an optional callback that fires synchronously right after the
// service is created, giving tests a race-free hook to register observers.
class AdBlockBrowserTestHelper {
 public:
  explicit AdBlockBrowserTestHelper(
      base::RepeatingClosure callback = base::DoNothing());
  AdBlockBrowserTestHelper(const AdBlockBrowserTestHelper&) = delete;
  AdBlockBrowserTestHelper& operator=(const AdBlockBrowserTestHelper&) = delete;
  ~AdBlockBrowserTestHelper();

  // Blocks until the initial empty-catalog default-engine filter list load
  // fires. Idempotent — calling more than once is a no-op. Call this before
  // registering any TestFiltersProvider so the subsequent observer only wakes
  // for the rule load (not for a stale initial update).
  //
  // NOT safe with the DAT cache feature enabled — a cached DAT load may
  // suppress the initial filter set build, making this wait hang. DAT cache
  // tests should skip calling it.
  void WaitForAdBlockEngineInitialLoad();

 private:
  void SetUpAdBlockService(content::BrowserContext* context);

  base::RepeatingClosure callback_;
  std::unique_ptr<AdBlockServiceTestObserver> initial_observer_;
  base::CallbackListSubscription create_services_subscription_;
};

}  // namespace brave_shields

#endif  // BRAVE_BROWSER_BRAVE_SHIELDS_AD_BLOCK_BROWSER_TEST_HELPER_H_
