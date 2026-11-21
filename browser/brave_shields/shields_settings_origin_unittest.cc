/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_shields/shields_settings_origin.h"

#include <memory>
#include <optional>

#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/test/navigation_simulator.h"
#include "content/public/test/test_renderer_host.h"
#include "net/http/http_response_headers.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace brave_shields {

namespace {

url::Origin A() {
  return url::Origin::Create(GURL("https://a.test"));
}

url::Origin B() {
  return url::Origin::Create(GURL("https://b.test"));
}

// Forwards navigation events to the `ShieldsSettingsOrigin` hooks, as
// `BraveShieldsWebContentsObserver` does.
class ShieldsSettingsOriginObserver : public content::WebContentsObserver {
 public:
  explicit ShieldsSettingsOriginObserver(content::WebContents* web_contents)
      : content::WebContentsObserver(web_contents) {}

  void DidStartNavigation(content::NavigationHandle* handle) override {
    ShieldsSettingsOrigin::OnNavigationStarted(handle);
  }
  void ReadyToCommitNavigation(content::NavigationHandle* handle) override {
    ShieldsSettingsOrigin::OnReadyToCommitNavigation(handle);
  }
  void DidFinishNavigation(content::NavigationHandle* handle) override {
    ShieldsSettingsOrigin::OnNavigationFinished(handle);
  }
};

}  // namespace

TEST(ShieldsSettingsOriginResolveTest, UsesNonOpaqueOrigin) {
  EXPECT_EQ(A(), ShieldsSettingsOrigin::Resolve(A(), GURL("https://b.test/"),
                                                url::Origin()));
}

TEST(ShieldsSettingsOriginResolveTest, UsesUrlOriginWhenOriginIsOpaque) {
  EXPECT_EQ(B(), ShieldsSettingsOrigin::Resolve(url::Origin(),
                                                GURL("https://b.test/"), A()));
}

TEST(ShieldsSettingsOriginResolveTest, UsesUrlOriginWhenOriginIsMissing) {
  EXPECT_EQ(B(), ShieldsSettingsOrigin::Resolve(std::nullopt,
                                                GURL("https://b.test/"), A()));
}

TEST(ShieldsSettingsOriginResolveTest,
     UsesInheritedWhenOriginAndUrlOriginAreOpaque) {
  EXPECT_EQ(A(), ShieldsSettingsOrigin::Resolve(url::Origin(),
                                                GURL("data:text/html,x"), A()));
}

TEST(ShieldsSettingsOriginResolveTest, OpaqueInheritedStaysOpaque) {
  EXPECT_TRUE(ShieldsSettingsOrigin::Resolve(std::nullopt, GURL("about:blank"),
                                             url::Origin())
                  .opaque());
}

class ShieldsSettingsOriginTest : public ChromeRenderViewHostTestHarness {
 protected:
  void SetUp() override {
    ChromeRenderViewHostTestHarness::SetUp();
    observer_ = std::make_unique<ShieldsSettingsOriginObserver>(web_contents());
  }

  void TearDown() override {
    observer_.reset();
    ChromeRenderViewHostTestHarness::TearDown();
  }

  std::unique_ptr<content::WebContents> CreateObservedContents() {
    auto contents = CreateTestWebContents();
    new_observers_.push_back(
        std::make_unique<ShieldsSettingsOriginObserver>(contents.get()));
    return contents;
  }

  url::Origin GetForMainFrame() {
    return ShieldsSettingsOrigin::GetForFrame(main_rfh());
  }

  std::unique_ptr<ShieldsSettingsOriginObserver> observer_;
  std::vector<std::unique_ptr<ShieldsSettingsOriginObserver>> new_observers_;
};

TEST_F(ShieldsSettingsOriginTest, GetForFrameWithoutValueUsesCommittedOrigin) {
  // No observer, so the document has no stored value.
  observer_.reset();
  content::NavigationSimulator::NavigateAndCommitFromBrowser(
      web_contents(), GURL("https://a.test/"));
  ASSERT_FALSE(ShieldsSettingsOrigin::GetForCurrentDocument(main_rfh()));

  EXPECT_EQ(A(), GetForMainFrame());
}

TEST_F(ShieldsSettingsOriginTest, GetForFrameUsesOutermostMainFrameValue) {
  content::NavigationSimulator::NavigateAndCommitFromBrowser(
      web_contents(), GURL("https://a.test/"));
  content::RenderFrameHost* child =
      content::RenderFrameHostTester::For(main_rfh())->AppendChild("child");
  child = content::NavigationSimulator::NavigateAndCommitFromDocument(
      GURL("https://b.test/"), child);

  EXPECT_EQ(A(), ShieldsSettingsOrigin::GetForFrame(child));
}

TEST_F(ShieldsSettingsOriginTest, CommitStoresCommittedOrigin) {
  content::NavigationSimulator::NavigateAndCommitFromBrowser(
      web_contents(), GURL("https://a.test/"));

  const auto* data = ShieldsSettingsOrigin::GetForCurrentDocument(main_rfh());
  ASSERT_TRUE(data);
  EXPECT_EQ(A(), data->value());
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

  EXPECT_TRUE(GetForMainFrame().opaque());
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

  ASSERT_FALSE(ShieldsSettingsOrigin::GetForCurrentDocument(child));
  EXPECT_EQ(A(), GetForMainFrame());
}

TEST_F(ShieldsSettingsOriginTest, NewWindowCopiesOpenerPageValue) {
  content::NavigationSimulator::NavigateAndCommitFromBrowser(
      web_contents(), GURL("https://a.test/"));
  auto new_contents = CreateObservedContents();

  ShieldsSettingsOrigin::SetForNewWindow(new_contents.get(), main_rfh());

  EXPECT_EQ(A(), ShieldsSettingsOrigin::GetForFrame(
                     new_contents->GetPrimaryMainFrame()));
}

TEST_F(ShieldsSettingsOriginTest, NewWindowValueSurvivesOpenerNavigation) {
  content::NavigationSimulator::NavigateAndCommitFromBrowser(
      web_contents(), GURL("https://a.test/"));
  auto new_contents = CreateObservedContents();
  ShieldsSettingsOrigin::SetForNewWindow(new_contents.get(), main_rfh());

  content::NavigationSimulator::NavigateAndCommitFromBrowser(
      web_contents(), GURL("https://b.test/"));

  EXPECT_EQ(A(), ShieldsSettingsOrigin::GetForFrame(
                     new_contents->GetPrimaryMainFrame()));
}

TEST_F(ShieldsSettingsOriginTest, NewWindowIgnoredAfterInitialNavigation) {
  content::NavigationSimulator::NavigateAndCommitFromBrowser(
      web_contents(), GURL("https://a.test/"));
  auto new_contents = CreateObservedContents();
  content::NavigationSimulator::NavigateAndCommitFromBrowser(
      new_contents.get(), GURL("https://b.test/"));

  ShieldsSettingsOrigin::SetForNewWindow(new_contents.get(), main_rfh());

  EXPECT_EQ(B(), ShieldsSettingsOrigin::GetForFrame(
                     new_contents->GetPrimaryMainFrame()));
}

TEST_F(ShieldsSettingsOriginTest, NewWindowOpaqueNavigationCopiesOpenerValue) {
  content::NavigationSimulator::NavigateAndCommitFromBrowser(
      web_contents(), GURL("https://a.test/"));
  auto new_contents = CreateObservedContents();
  ShieldsSettingsOrigin::SetForNewWindow(new_contents.get(), main_rfh());

  auto simulator = content::NavigationSimulator::CreateRendererInitiated(
      GURL("about:blank#x"), new_contents->GetPrimaryMainFrame());
  simulator->SetInitiatorFrame(new_contents->GetPrimaryMainFrame());
  simulator->Commit();

  EXPECT_EQ(A(), ShieldsSettingsOrigin::GetForFrame(
                     new_contents->GetPrimaryMainFrame()));
}

}  // namespace brave_shields
