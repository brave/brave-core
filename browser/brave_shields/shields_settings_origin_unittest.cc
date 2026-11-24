/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_shields/shields_settings_origin.h"

#include <memory>

#include "brave/browser/brave_shields/brave_shields_web_contents_observer.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/referrer.h"
#include "content/public/test/navigation_simulator.h"
#include "content/public/test/test_renderer_host.h"
#include "net/http/http_response_headers.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/window_open_disposition.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace brave_shields {

namespace {

GURL A() {
  return GURL("https://a.test/");
}
GURL B() {
  return GURL("https://b.test/");
}

}  // namespace

class ShieldsSettingsOriginTest : public ChromeRenderViewHostTestHarness {
 protected:
  void SetUp() override {
    ChromeRenderViewHostTestHarness::SetUp();
    BraveShieldsWebContentsObserver::CreateForWebContents(web_contents());
  }

  std::unique_ptr<content::WebContents> CreateObservedContents() {
    auto contents = CreateTestWebContents();
    BraveShieldsWebContentsObserver::CreateForWebContents(contents.get());
    return contents;
  }

  void OpenWindow(content::WebContents* contents) {
    content::WebContentsObserver* observer =
        BraveShieldsWebContentsObserver::FromWebContents(web_contents());
    observer->DidOpenRequestedURL(contents, main_rfh(), GURL(),
                                  content::Referrer(),
                                  WindowOpenDisposition::kNewPopup,
                                  ui::PAGE_TRANSITION_LINK, false, true);
  }

  GURL GetForMainFrame() {
    return ShieldsSettingsOrigin::ForFrame(*main_rfh()).GetURL();
  }
};

TEST_F(ShieldsSettingsOriginTest, GetForFrameWithoutValueUsesCommittedOrigin) {
  // No observer, so the document has no stored value.
  auto contents = CreateTestWebContents();
  content::NavigationSimulator::NavigateAndCommitFromBrowser(contents.get(),
                                                             A());
  EXPECT_EQ(A(),
            ShieldsSettingsOrigin::ForFrame(*contents->GetPrimaryMainFrame())
                .GetURL());
}

TEST_F(ShieldsSettingsOriginTest, GetForFrameUsesOutermostMainFrameValue) {
  content::NavigationSimulator::NavigateAndCommitFromBrowser(
      web_contents(), GURL("https://a.test/"));
  content::RenderFrameHost* child =
      content::RenderFrameHostTester::For(main_rfh())->AppendChild("child");
  child = content::NavigationSimulator::NavigateAndCommitFromDocument(
      GURL("https://b.test/"), child);

  EXPECT_EQ(A(), ShieldsSettingsOrigin::ForFrame(*child).GetURL());
}

TEST_F(ShieldsSettingsOriginTest, CommitStoresCommittedOrigin) {
  content::NavigationSimulator::NavigateAndCommitFromBrowser(
      web_contents(), GURL("https://a.test/"));

  EXPECT_EQ(A(), GetForMainFrame());
}

TEST_F(ShieldsSettingsOriginTest, SandboxedResponseStoresUrlOrigin) {
  auto simulator = content::NavigationSimulator::CreateBrowserInitiated(
      GURL("https://a.test/"), web_contents());
  simulator->SetResponseHeaders(net::HttpResponseHeaders::TryToCreate(
      "HTTP/1.1 200 OK\nContent-Security-Policy: sandbox allow-scripts\n"));
  simulator->Commit();
  ASSERT_TRUE(main_rfh()->GetLastCommittedOrigin().opaque());

  EXPECT_EQ(A(), GetForMainFrame());
}

TEST_F(ShieldsSettingsOriginTest, BrowserInitiatedOpaqueNavigationIsOpaque) {
  content::NavigationSimulator::NavigateAndCommitFromBrowser(
      web_contents(), GURL("https://a.test/"));
  content::NavigationSimulator::NavigateAndCommitFromBrowser(
      web_contents(), GURL("data:text/html,x"));

  EXPECT_TRUE(GetForMainFrame().is_empty());
}

TEST_F(ShieldsSettingsOriginTest,
       RendererInitiatedOpaqueNavigationCopiesInitiatorValue) {
  content::NavigationSimulator::NavigateAndCommitFromBrowser(
      web_contents(), GURL("https://a.test/"));
  auto simulator = content::NavigationSimulator::CreateRendererInitiated(
      GURL("about:blank"), main_rfh());
  simulator->SetInitiatorFrame(main_rfh());
  simulator->Commit();

  EXPECT_EQ(A(), GetForMainFrame());
}

TEST_F(ShieldsSettingsOriginTest,
       RendererInitiatedOpaqueNavigationCopiesInitiatorPageValue) {
  content::NavigationSimulator::NavigateAndCommitFromBrowser(
      web_contents(), GURL("https://a.test/"));
  content::RenderFrameHost* child =
      content::RenderFrameHostTester::For(main_rfh())->AppendChild("child");
  child = content::NavigationSimulator::NavigateAndCommitFromDocument(
      GURL("https://b.test/"), child);

  auto simulator = content::NavigationSimulator::CreateRendererInitiated(
      GURL("about:blank"), main_rfh());
  simulator->SetInitiatorFrame(child);
  simulator->Commit();

  // The subframe's page is a.test, not the subframe's own b.test.
  EXPECT_EQ(A(), GetForMainFrame());
}

TEST_F(ShieldsSettingsOriginTest, NonOpaqueNavigationIgnoresInitiatorValue) {
  content::NavigationSimulator::NavigateAndCommitFromBrowser(
      web_contents(), GURL("https://a.test/"));
  auto simulator = content::NavigationSimulator::CreateRendererInitiated(
      GURL("https://b.test/"), main_rfh());
  simulator->SetInitiatorFrame(main_rfh());
  simulator->Commit();

  EXPECT_EQ(B(), GetForMainFrame());
}

TEST_F(ShieldsSettingsOriginTest, SubframeNavigationDoesNotChangeValue) {
  content::NavigationSimulator::NavigateAndCommitFromBrowser(
      web_contents(), GURL("https://a.test/"));
  content::RenderFrameHost* child =
      content::RenderFrameHostTester::For(main_rfh())->AppendChild("child");
  child = content::NavigationSimulator::NavigateAndCommitFromDocument(
      GURL("https://b.test/"), child);

  EXPECT_EQ(A(), GetForMainFrame());
}

TEST_F(ShieldsSettingsOriginTest, NewWindowCopiesOpenerPageValue) {
  content::NavigationSimulator::NavigateAndCommitFromBrowser(
      web_contents(), GURL("https://a.test/"));
  auto new_contents = CreateObservedContents();

  OpenWindow(new_contents.get());

  EXPECT_EQ(
      A(), ShieldsSettingsOrigin::ForFrame(*new_contents->GetPrimaryMainFrame())
               .GetURL());
}

TEST_F(ShieldsSettingsOriginTest, NewWindowValueSurvivesOpenerNavigation) {
  content::NavigationSimulator::NavigateAndCommitFromBrowser(
      web_contents(), GURL("https://a.test/"));
  auto new_contents = CreateObservedContents();
  OpenWindow(new_contents.get());

  content::NavigationSimulator::NavigateAndCommitFromBrowser(
      web_contents(), GURL("https://b.test/"));

  EXPECT_EQ(
      A(), ShieldsSettingsOrigin::ForFrame(*new_contents->GetPrimaryMainFrame())
               .GetURL());
}

TEST_F(ShieldsSettingsOriginTest, NewWindowIgnoredAfterInitialNavigation) {
  content::NavigationSimulator::NavigateAndCommitFromBrowser(
      web_contents(), GURL("https://a.test/"));
  auto new_contents = CreateObservedContents();
  content::NavigationSimulator::NavigateAndCommitFromBrowser(
      new_contents.get(), GURL("https://b.test/"));

  OpenWindow(new_contents.get());

  EXPECT_EQ(
      B(), ShieldsSettingsOrigin::ForFrame(*new_contents->GetPrimaryMainFrame())
               .GetURL());
}

TEST_F(ShieldsSettingsOriginTest, NewWindowOpaqueNavigationCopiesOpenerValue) {
  content::NavigationSimulator::NavigateAndCommitFromBrowser(
      web_contents(), GURL("https://a.test/"));
  auto new_contents = CreateObservedContents();
  OpenWindow(new_contents.get());

  auto simulator = content::NavigationSimulator::CreateRendererInitiated(
      GURL("about:blank#x"), new_contents->GetPrimaryMainFrame());
  simulator->SetInitiatorFrame(new_contents->GetPrimaryMainFrame());
  simulator->Commit();

  EXPECT_EQ(
      A(), ShieldsSettingsOrigin::ForFrame(*new_contents->GetPrimaryMainFrame())
               .GetURL());
}

}  // namespace brave_shields
