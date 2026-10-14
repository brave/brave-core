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
// navigating uses `BraveWebUIControllerFactory::GetWebUIType`, which requires
// a real `Profile`-backed `BrowserContext`, not just any `BrowserContext`.
using NavigationCompletionWaiterTest = ChromeRenderViewHostTestHarness;

TEST_F(NavigationCompletionWaiterTest,
       ShouldRunCallbackOnceWatchedNavigationCommitsAndStopsLoading) {
  base::test::TestFuture<void> on_done;
  NavigationCompletionWaiter::Watch(web_contents(), on_done.GetCallback());

  // `NavigateAndCommitFromBrowser` stops loading as part of its own commit,
  // collapsing the two events this test means to tell apart; drives the
  // simulator manually instead, keeping the navigation loading through
  // commit so stopping it afterward is a separate, observable step.
  std::unique_ptr<content::NavigationSimulator> navigation =
      content::NavigationSimulator::CreateBrowserInitiated(
          GURL("https://brave.com/"), web_contents());
  navigation->SetKeepLoading(true);
  navigation->Commit();
  EXPECT_FALSE(on_done.IsReady());

  navigation->StopLoading();

  EXPECT_TRUE(on_done.Wait());
}

TEST_F(NavigationCompletionWaiterTest,
       ShouldNotRunCallbackForNavigationAlreadyInProgressWhenWatchingStarts) {
  // Simulates a navigation already underway on this `WebContents` (e.g. a
  // brand new tab's own default page) when `Watch` is called for a later,
  // unrelated chained action.
  std::unique_ptr<content::NavigationSimulator> in_flight_navigation =
      content::NavigationSimulator::CreateBrowserInitiated(
          GURL("https://example.com/"), web_contents());
  in_flight_navigation->Start();

  base::test::TestFuture<void> on_done;
  NavigationCompletionWaiter::Watch(web_contents(), on_done.GetCallback());

  in_flight_navigation->Commit();
  content::WebContentsTester::For(web_contents())->TestSetIsLoading(false);

  EXPECT_FALSE(on_done.IsReady());
}

TEST_F(NavigationCompletionWaiterTest,
       ShouldReplacePreviousWaiterWhenWatchingAgain) {
  base::test::TestFuture<void> first_on_done;
  base::test::TestFuture<void> second_on_done;
  NavigationCompletionWaiter::Watch(web_contents(),
                                    first_on_done.GetCallback());

  // A second `Watch` call before the first finished (e.g. a later chained
  // action overtaking an earlier one) replaces it outright, rather than
  // leaving two waiters racing to finish the same chain.
  NavigationCompletionWaiter::Watch(web_contents(),
                                    second_on_done.GetCallback());

  content::NavigationSimulator::NavigateAndCommitFromBrowser(
      web_contents(), GURL("https://brave.com/"));
  content::WebContentsTester::For(web_contents())->TestSetIsLoading(false);

  EXPECT_TRUE(second_on_done.Wait());
  EXPECT_FALSE(first_on_done.IsReady());
}

}  // namespace gestures
