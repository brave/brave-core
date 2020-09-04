/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ipfs/ipfs_navigation_throttle.h"

#include <memory>
#include <vector>

#include "base/test/bind_test_util.h"
#include "brave/browser/ipfs/ipfs_service.h"
#include "brave/browser/ipfs/ipfs_service_factory.h"
#include "brave/common/pref_names.h"
#include "brave/components/ipfs/common/ipfs_constants.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "chrome/test/base/scoped_testing_local_state.h"
#include "chrome/test/base/testing_browser_process.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/common/content_client.h"
#include "content/public/test/mock_navigation_handle.h"
#include "content/public/test/web_contents_tester.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

using content::NavigationThrottle;

namespace {

class MockBrowserClient : public content::ContentBrowserClient {
 public:
  MockBrowserClient() {}
  ~MockBrowserClient() override {}

  // Only construct an IpfsNavigationThrottle so that we can test it in
  // isolation.
  std::vector<std::unique_ptr<NavigationThrottle>> CreateThrottlesForNavigation(
      content::NavigationHandle* handle) override {
    std::vector<std::unique_ptr<NavigationThrottle>> throttles;
    throttles.push_back(
        std::make_unique<ipfs::IpfsNavigationThrottle>(handle));
    return throttles;
  }
};

const GURL& GetIPFSURL() {
  static const GURL ipfs_url(
      "http://localhost:8080/ipfs/bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq/wiki/Vincent_van_Gogh.html");  // NOLINT
  return ipfs_url;
}

const GURL& GetIPNSURL() {
  static const GURL ipns_url(
      "http://localhost:8080/ipns/tr.wikipedia-on-ipfs.org/wiki/Anasayfa.html");  // NOLINT
  return ipns_url;
}

}  // namespace

namespace ipfs {

class IpfsNavigationThrottleUnitTest : public ChromeRenderViewHostTestHarness {
 public:
  IpfsNavigationThrottleUnitTest()
    :local_state_(TestingBrowserProcess::GetGlobal()) {}
  ~IpfsNavigationThrottleUnitTest() override = default;

  void SetUp() override {
    original_client_ = content::SetBrowserClientForTesting(&client_);
    ChromeRenderViewHostTestHarness::SetUp();
     web_contents_ =
         content::WebContentsTester::CreateTestWebContents(profile(), nullptr);
    ipfs_service_ = IpfsServiceFactory::GetForContext(profile());
  }

  void TearDown() override {
    content::SetBrowserClientForTesting(original_client_);
    web_contents_.reset();
    ChromeRenderViewHostTestHarness::TearDown();
  }

  content::WebContents* web_contents() {
    return web_contents_.get();
  }

  IpfsService* ipfs_service() {
    return ipfs_service_;
  }

 private:
  MockBrowserClient client_;
  content::ContentBrowserClient* original_client_;
  ScopedTestingLocalState local_state_;
  std::unique_ptr<content::WebContents> web_contents_;
  IpfsService* ipfs_service_;

  DISALLOW_COPY_AND_ASSIGN(IpfsNavigationThrottleUnitTest);
};

TEST_F(IpfsNavigationThrottleUnitTest, DeferUntilIpfsProcessLaunched) {
  profile()->GetPrefs()->SetInteger(kIPFSResolveMethod,
      static_cast<int>(IPFSResolveMethodTypes::IPFS_LOCAL));

  content::MockNavigationHandle test_handle(web_contents());
  test_handle.set_url(GetIPFSURL());
  auto throttle = std::make_unique<IpfsNavigationThrottle>(&test_handle);
  bool was_navigation_resumed = false;
  throttle->set_resume_callback_for_testing(
      base::BindLambdaForTesting([&]() { was_navigation_resumed = true; }));
  EXPECT_EQ(NavigationThrottle::DEFER, throttle->WillStartRequest().action())
      << GetIPFSURL();
  throttle->OnIpfsLaunched(true, 5566);
  EXPECT_TRUE(was_navigation_resumed);
  ipfs_service()->SetIpfsLaunchedForTest(true);
  EXPECT_EQ(NavigationThrottle::PROCEED, throttle->WillStartRequest().action())
      << GetIPFSURL();

  ipfs_service()->SetIpfsLaunchedForTest(false);

  was_navigation_resumed = false;
  test_handle.set_url(GetIPNSURL());
  EXPECT_EQ(NavigationThrottle::DEFER, throttle->WillStartRequest().action())
      << GetIPNSURL();
  throttle->OnIpfsLaunched(true, 5566);
  EXPECT_TRUE(was_navigation_resumed);
  ipfs_service()->SetIpfsLaunchedForTest(true);
  EXPECT_EQ(NavigationThrottle::PROCEED, throttle->WillStartRequest().action())
      << GetIPNSURL();
}

TEST_F(IpfsNavigationThrottleUnitTest, ProceedForGatewayNodeMode) {
  profile()->GetPrefs()->SetInteger(kIPFSResolveMethod,
      static_cast<int>(IPFSResolveMethodTypes::IPFS_GATEWAY));

  content::MockNavigationHandle test_handle(web_contents());
  test_handle.set_url(GetIPFSURL());
  auto throttle = std::make_unique<IpfsNavigationThrottle>(&test_handle);
  EXPECT_EQ(NavigationThrottle::PROCEED, throttle->WillStartRequest().action())
      << GetIPFSURL();

  profile()->GetPrefs()->SetInteger(kIPFSResolveMethod,
      static_cast<int>(IPFSResolveMethodTypes::IPFS_DISABLED));
  EXPECT_EQ(NavigationThrottle::PROCEED, throttle->WillStartRequest().action())
      << GetIPFSURL();
}

TEST_F(IpfsNavigationThrottleUnitTest, ProceedForAskNodeMode) {
  profile()->GetPrefs()->SetInteger(kIPFSResolveMethod,
      static_cast<int>(IPFSResolveMethodTypes::IPFS_ASK));

  content::MockNavigationHandle test_handle(web_contents());
  test_handle.set_url(GetIPFSURL());
  auto throttle = std::make_unique<IpfsNavigationThrottle>(&test_handle);
  EXPECT_EQ(NavigationThrottle::PROCEED, throttle->WillStartRequest().action())
      << GetIPFSURL();

  profile()->GetPrefs()->SetInteger(kIPFSResolveMethod,
      static_cast<int>(IPFSResolveMethodTypes::IPFS_DISABLED));
  EXPECT_EQ(NavigationThrottle::PROCEED, throttle->WillStartRequest().action())
      << GetIPFSURL();
}

}  // namespace ipfs
