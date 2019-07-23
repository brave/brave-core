/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/brave_wallet_navigation_throttle.h"

#include <memory>
#include <vector>

#include "brave/common/extensions/extension_constants.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/common/content_client.h"
#include "content/public/test/mock_navigation_handle.h"
#include "content/public/test/web_contents_tester.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_builder.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

using content::NavigationThrottle;

namespace extensions {

namespace {

class MockBrowserClient : public content::ContentBrowserClient {
 public:
  MockBrowserClient() {}
  ~MockBrowserClient() override {}

  // Only construct an BraveWalletNavigationThrottle so that we can test it in
  // isolation.
  std::vector<std::unique_ptr<NavigationThrottle>> CreateThrottlesForNavigation(
      content::NavigationHandle* handle) override {
    std::vector<std::unique_ptr<NavigationThrottle>> throttles;
    throttles.push_back(
        std::make_unique<BraveWalletNavigationThrottle>(handle));
    return throttles;
  }
};

}  // namespace

class BraveWalletNavigationThrottleUnitTest
    : public ChromeRenderViewHostTestHarness {
 public:
  BraveWalletNavigationThrottleUnitTest() {}

  void SetUp() override {
    ChromeRenderViewHostTestHarness::SetUp();
    original_client_ = content::SetBrowserClientForTesting(&client_);
  }

  void TearDown() override {
    content::SetBrowserClientForTesting(original_client_);
    ChromeRenderViewHostTestHarness::TearDown();
  }

  content::RenderFrameHostTester* render_frame_host_tester(
      content::RenderFrameHost* host) {
    return content::RenderFrameHostTester::For(host);
  }

  content::WebContentsTester* web_contents_tester() {
    return content::WebContentsTester::For(web_contents());
  }

  void AddExtension() {
    DictionaryBuilder manifest;
    manifest.Set("name", "ext")
        .Set("version", "0.1")
        .Set("manifest_version", 2);
    extension_ = ExtensionBuilder()
                     .SetManifest(manifest.Build())
                     .SetID(ethereum_remote_client_extension_id)
                     .Build();
    ASSERT_TRUE(extension_);
    ExtensionRegistry::Get(browser_context())->AddReady(extension_.get());
  }

 private:
  scoped_refptr<const Extension> extension_;
  MockBrowserClient client_;
  content::ContentBrowserClient* original_client_;
  DISALLOW_COPY_AND_ASSIGN(BraveWalletNavigationThrottleUnitTest);
};

// Tests the basic case of loading a URL, it should proceed.
TEST_F(BraveWalletNavigationThrottleUnitTest, ExternalWebPage) {
  web_contents_tester()->NavigateAndCommit(GURL("http://example.com"));
  content::RenderFrameHost* host =
      render_frame_host_tester(main_rfh())->AppendChild("child");
  GURL url("http://www.example.com");
  content::MockNavigationHandle test_handle(url, host);
  test_handle.set_starting_site_instance(host->GetSiteInstance());
  auto throttle = std::make_unique<BraveWalletNavigationThrottle>(&test_handle);
  EXPECT_EQ(NavigationThrottle::PROCEED, throttle->WillStartRequest().action())
      << url;
}

// Tests the case of loading brave://settings without the extension installed.
// It should just proceed.
TEST_F(BraveWalletNavigationThrottleUnitTest, DifferentChromePageWithExt) {
  web_contents_tester()->NavigateAndCommit(GURL("http://example.com"));
  content::RenderFrameHost* host =
      render_frame_host_tester(main_rfh())->AppendChild("child");
  GURL url("chrome://settings");
  content::MockNavigationHandle test_handle(url, host);
  test_handle.set_starting_site_instance(host->GetSiteInstance());
  auto throttle = std::make_unique<BraveWalletNavigationThrottle>(&test_handle);
  EXPECT_EQ(NavigationThrottle::PROCEED, throttle->WillStartRequest().action())
      << url;
}

// Tests the case of loading brave://wallet without having the extension
// installed.a It should defer which it does to install the extension.
TEST_F(BraveWalletNavigationThrottleUnitTest, ChromeWalletUrlNotInstalled) {
  web_contents_tester()->NavigateAndCommit(GURL("http://example.com"));
  content::RenderFrameHost* host =
      render_frame_host_tester(main_rfh())->AppendChild("child");
  GURL url("chrome://wallet");
  content::MockNavigationHandle test_handle(url, host);
  test_handle.set_starting_site_instance(host->GetSiteInstance());
  auto throttle = std::make_unique<BraveWalletNavigationThrottle>(&test_handle);
  EXPECT_EQ(NavigationThrottle::DEFER, throttle->WillStartRequest().action())
      << url;
}

// Tests the case of loading brave://wallet with the extension installed.
// It should just proceed.
TEST_F(BraveWalletNavigationThrottleUnitTest, ChromeWalletUrlInstalled) {
  AddExtension();
  web_contents_tester()->NavigateAndCommit(GURL("http://example.com"));
  content::RenderFrameHost* host =
      render_frame_host_tester(main_rfh())->AppendChild("child");
  GURL url("chrome://wallet");
  content::MockNavigationHandle test_handle(url, host);
  test_handle.set_starting_site_instance(host->GetSiteInstance());
  auto throttle = std::make_unique<BraveWalletNavigationThrottle>(&test_handle);
  EXPECT_EQ(NavigationThrottle::PROCEED, throttle->WillStartRequest().action())
      << url;
}

}  // namespace extensions
