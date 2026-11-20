// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/decentralized_dns/content/decentralized_dns_navigation_throttle.h"

#include <memory>
#include <string>

#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/decentralized_dns/core/utils.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/mock_navigation_handle.h"
#include "content/public/test/mock_navigation_throttle_registry.h"
#include "content/public/test/test_browser_context.h"
#include "content/public/test/test_renderer_host.h"
#include "content/public/test/web_contents_tester.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace {

constexpr char kExampleURL[] = "https://example.com";
constexpr char kLocale[] = "en-US";

}  // namespace

namespace decentralized_dns {

class DecentralizedDnsNavigationThrottleTest
    : public content::RenderViewHostTestHarness {
 public:
  DecentralizedDnsNavigationThrottleTest() = default;
  ~DecentralizedDnsNavigationThrottleTest() override = default;

  void SetUp() override {
    content::RenderViewHostTestHarness::SetUp();
    RegisterLocalStatePrefs(local_state_.registry());
    user_prefs_.registry()->RegisterBooleanPref(
        brave_wallet::kBraveWalletDisabledByPolicy, false);
  }

  PrefService* user_prefs() { return &user_prefs_; }
  PrefService* local_state() { return &local_state_; }
  std::string locale() { return kLocale; }

 private:
  TestingPrefServiceSimple user_prefs_;
  TestingPrefServiceSimple local_state_;
};

TEST_F(DecentralizedDnsNavigationThrottleTest, Instantiation) {
  content::MockNavigationHandle test_handle(web_contents());
  content::MockNavigationThrottleRegistry registry(
      &test_handle,
      content::MockNavigationThrottleRegistry::RegistrationMode::kHold);
  DecentralizedDnsNavigationThrottle::MaybeCreateAndAdd(
      registry, user_prefs(), local_state(), locale());
  EXPECT_FALSE(registry.throttles().empty());
}

TEST_F(DecentralizedDnsNavigationThrottleTest, NotInstantiatedInOffTheRecord) {
  content::TestBrowserContext otr_context;
  otr_context.set_is_off_the_record(true);
  auto otr_web_contents =
      content::WebContentsTester::CreateTestWebContents(&otr_context, nullptr);
  content::MockNavigationHandle otr_test_handle(otr_web_contents.get());
  content::MockNavigationThrottleRegistry otr_registry(
      &otr_test_handle,
      content::MockNavigationThrottleRegistry::RegistrationMode::kHold);
  DecentralizedDnsNavigationThrottle::MaybeCreateAndAdd(
      otr_registry, user_prefs(), local_state(), locale());
  EXPECT_TRUE(otr_registry.throttles().empty());
}

TEST_F(DecentralizedDnsNavigationThrottleTest, Subframe) {
  content::RenderFrameHostTester::For(main_rfh())
      ->InitializeRenderFrameIfNeeded();
  content::RenderFrameHost* subframe =
      content::RenderFrameHostTester::For(main_rfh())->AppendChild("subframe");

  // Throttle is created for main frame.
  {
    content::MockNavigationHandle handle(GURL(kExampleURL), main_rfh());
    content::MockNavigationThrottleRegistry registry(
        &handle,
        content::MockNavigationThrottleRegistry::RegistrationMode::kHold);
    DecentralizedDnsNavigationThrottle::MaybeCreateAndAdd(
        registry, user_prefs(), local_state(), locale());
    EXPECT_FALSE(registry.throttles().empty());
  }
  // Throttle is not created for subframe.
  {
    content::MockNavigationHandle handle(GURL(kExampleURL), subframe);
    content::MockNavigationThrottleRegistry registry(
        &handle,
        content::MockNavigationThrottleRegistry::RegistrationMode::kHold);
    DecentralizedDnsNavigationThrottle::MaybeCreateAndAdd(
        registry, user_prefs(), local_state(), locale());
    EXPECT_TRUE(registry.throttles().empty());
  }
}

}  // namespace decentralized_dns
