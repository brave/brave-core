/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/brave_vpn_service_observer.h"

#include <memory>

#include "brave/components/brave_vpn/browser/test/fake_brave_vpn_service.h"
#include "content/public/test/browser_task_environment.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_vpn {

class BraveVpnServiceObserverTest : public testing::Test {
 public:
  BraveVpnServiceObserverTest() = default;
  ~BraveVpnServiceObserverTest() override = default;

 protected:
  void SetUp() override { service_ = std::make_unique<FakeBraveVpnService>(); }

  FakeBraveVpnService* service() { return service_.get(); }

 private:
  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<FakeBraveVpnService> service_;
};

// Calling Observe() twice on the same service should not crash,
// and the observer should be correctly re-bound after the second call.
TEST_F(BraveVpnServiceObserverTest, ObserveCalledTwiceDoesNotCrash) {
  FakeBraveVpnService* service = this->service();
  BraveVpnServiceObserver observer;
  EXPECT_NO_FATAL_FAILURE(observer.Observe(service));
  EXPECT_NO_FATAL_FAILURE(observer.Observe(service));
}

// Calling Observe() with a null service should be a safe no-op.
TEST_F(BraveVpnServiceObserverTest, ObserveWithNullServiceDoesNotCrash) {
  BraveVpnServiceObserver observer;
  EXPECT_NO_FATAL_FAILURE(observer.Observe(nullptr));
}

}  // namespace brave_vpn
