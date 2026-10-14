// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/gestures/dispatch/navigation_completion_waiter.h"

#include <memory>

#include "base/test/test_future.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "content/public/test/navigation_simulator.h"
#include "content/public/test/web_contents_tester.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace gestures {

// `ChromeRenderViewHostTestHarness`, not the plain `content` one, since
// navigating needs a `Profile`-backed `BrowserContext`.
class NavigationCompletionWaiterTest : public ChromeRenderViewHostTestHarness {
};

TEST_F(NavigationCompletionWaiterTest,
       ShouldRunCallbackAfterWatchedNavigationCommitsAndStopsLoading) {
  base::test::TestFuture<void> test_future;
  NavigationCompletionWaiter::Watch(web_contents(), test_future.GetCallback());

  // `NavigateAndCommitFromBrowser` stops loading as part of its own commit;
  // drives the simulator manually to keep commit and stop-loading separate.
  std::unique_ptr<content::NavigationSimulator> navigation =
      content::NavigationSimulator::CreateBrowserInitiated(
          GURL("https://brave.com/"), web_contents());
  navigation->SetKeepLoading(true);
  navigation->Commit();
  EXPECT_FALSE(test_future.IsReady());

  navigation->StopLoading();

  EXPECT_TRUE(test_future.Wait());
}

TEST_F(NavigationCompletionWaiterTest,
       ShouldNotRunCallbackForNavigationAlreadyInProgressWhenWatchingStarts) {
  // Simulates a navigation already underway when `Watch` is called for a
  // later, unrelated chained action.
  std::unique_ptr<content::NavigationSimulator> in_flight_navigation =
      content::NavigationSimulator::CreateBrowserInitiated(
          GURL("https://foo.com/"), web_contents());
  in_flight_navigation->Start();

  base::test::TestFuture<void> test_future;
  NavigationCompletionWaiter::Watch(web_contents(), test_future.GetCallback());

  in_flight_navigation->Commit();
  content::WebContentsTester::For(web_contents())->TestSetIsLoading(false);

  EXPECT_FALSE(test_future.IsReady());
}

TEST_F(NavigationCompletionWaiterTest,
       ShouldReplacePreviousWaiterWhenWatchingAgain) {
  base::test::TestFuture<void> first_test_future;
  base::test::TestFuture<void> second_test_future;
  NavigationCompletionWaiter::Watch(web_contents(),
                                    first_test_future.GetCallback());

  // A second `Watch` call before the first finished replaces it outright,
  // rather than leaving two waiters racing to finish the same chain.
  NavigationCompletionWaiter::Watch(web_contents(),
                                    second_test_future.GetCallback());

  content::NavigationSimulator::NavigateAndCommitFromBrowser(
      web_contents(), GURL("https://brave.com/"));
  content::WebContentsTester::For(web_contents())->TestSetIsLoading(false);

  EXPECT_TRUE(second_test_future.Wait());
  EXPECT_FALSE(first_test_future.IsReady());
}

}  // namespace gestures
