/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_shields/brave_shields_web_contents_observer.h"

#include "base/memory/raw_ptr.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "content/public/test/mock_navigation_handle.h"
#include "content/public/test/navigation_simulator.h"
#include "content/public/test/test_renderer_host.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace brave_shields {

class BraveShieldsWebContentsObserverUnitTest
    : public ChromeRenderViewHostTestHarness {
 public:
  BraveShieldsWebContentsObserverUnitTest() = default;
  ~BraveShieldsWebContentsObserverUnitTest() override = default;

  void SetUp() override {
    ChromeRenderViewHostTestHarness::SetUp();
    BraveShieldsWebContentsObserver::CreateForWebContents(web_contents());
    observer_ =
        BraveShieldsWebContentsObserver::FromWebContents(web_contents());
  }

  void TearDown() override {
    observer_ = nullptr;
    ChromeRenderViewHostTestHarness::TearDown();
  }

 protected:
  GURL GetPrimaryUrlFromHandle(content::NavigationHandle* handle) {
    return observer_->GetPrimaryUrlFromHandle(handle);
  }

  raw_ptr<BraveShieldsWebContentsObserver> observer_ = nullptr;
};

TEST_F(BraveShieldsWebContentsObserverUnitTest,
       MainFrameUsesOriginOfNavigationUrl) {
  content::MockNavigationHandle handle(GURL("https://example.com/path?q=1"),
                                       main_rfh());

  EXPECT_EQ(GURL("https://example.com/"), GetPrimaryUrlFromHandle(&handle));
}

TEST_F(BraveShieldsWebContentsObserverUnitTest,
       MainFrameBlobUrlUsesEmbeddedOrigin) {
  content::MockNavigationHandle handle(
      GURL("blob:https://example.com/550e8400-e29b-41d4-a716-446655440000"),
      main_rfh());

  EXPECT_EQ(GURL("https://example.com/"), GetPrimaryUrlFromHandle(&handle));
}

TEST_F(BraveShieldsWebContentsObserverUnitTest, MainFrameBraveUrlUsesOrigin) {
  content::MockNavigationHandle handle(GURL("brave://version"), main_rfh());

  EXPECT_EQ(GURL("brave://version/"), GetPrimaryUrlFromHandle(&handle));
}

TEST_F(BraveShieldsWebContentsObserverUnitTest,
       MainFrameAboutBlankFallsBackToNavigationUrl) {
  const GURL about_blank("about:blank");
  content::MockNavigationHandle handle(about_blank, main_rfh());

  EXPECT_EQ(about_blank, GetPrimaryUrlFromHandle(&handle));
}

TEST_F(BraveShieldsWebContentsObserverUnitTest,
       MainFrameOpaqueOriginFallsBackToNavigationUrl) {
  const GURL data_url("data:text/html,hello");
  content::MockNavigationHandle handle(data_url, main_rfh());

  EXPECT_EQ(data_url, GetPrimaryUrlFromHandle(&handle));
}

TEST_F(BraveShieldsWebContentsObserverUnitTest,
       SubframeUsesOutermostMainFrameOrigin) {
  const GURL main_url("https://example.com/page");
  content::NavigationSimulator::NavigateAndCommitFromBrowser(web_contents(),
                                                             main_url);

  content::RenderFrameHost* subframe =
      content::RenderFrameHostTester::For(main_rfh())->AppendChild("subframe");
  content::MockNavigationHandle handle(GURL("https://ads.example/tracker"),
                                       subframe);

  EXPECT_EQ(GURL("https://example.com/"), GetPrimaryUrlFromHandle(&handle));
}

TEST_F(BraveShieldsWebContentsObserverUnitTest,
       SubframeAboutBlankUsesOutermostMainFrameOrigin) {
  content::NavigationSimulator::NavigateAndCommitFromBrowser(
      web_contents(), GURL("https://example.com/page"));

  content::RenderFrameHost* subframe =
      content::RenderFrameHostTester::For(main_rfh())->AppendChild("subframe");
  content::MockNavigationHandle handle(GURL("about:blank"), subframe);

  EXPECT_EQ(GURL("https://example.com/"), GetPrimaryUrlFromHandle(&handle));
}

TEST_F(BraveShieldsWebContentsObserverUnitTest,
       SubframeOpaqueMainFrameOriginFallsBackToNavigationUrl) {
  content::NavigationSimulator::NavigateAndCommitFromBrowser(
      web_contents(), GURL("data:text/html,parent"));
  ASSERT_TRUE(
      web_contents()->GetPrimaryMainFrame()->GetLastCommittedOrigin().opaque());

  content::RenderFrameHost* subframe =
      content::RenderFrameHostTester::For(main_rfh())->AppendChild("subframe");
  const GURL subframe_url("https://embedded.example/frame");
  content::MockNavigationHandle handle(subframe_url, subframe);

  EXPECT_EQ(subframe_url, GetPrimaryUrlFromHandle(&handle));
}

}  // namespace brave_shields
