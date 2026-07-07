/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/v2/purchased_state_manager.h"

#include <memory>
#include <optional>
#include <string>

#include "base/functional/callback_helpers.h"
#include "brave/components/brave_vpn/common/brave_vpn_utils.h"
#include "brave/components/brave_vpn/common/mojom/brave_vpn.mojom.h"
#include "brave/components/brave_vpn/common/pref_names.h"
#include "brave/components/skus/browser/skus_utils.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_vpn::v2 {
namespace {
constexpr char kTestEnvironment[] = "unittest-env";
}  // namespace

class PurchasedStateManagerTest : public testing::Test {
 public:
  PurchasedStateManagerTest() {
    brave_vpn::RegisterLocalStatePrefs(local_pref_service_.registry());
  }

  void CreateManager() {
    manager_ = std::make_unique<PurchasedStateManager>(&local_pref_service_,
                                                       base::DoNothing());
  }

 protected:
  TestingPrefServiceSimple local_pref_service_;
  std::unique_ptr<PurchasedStateManager> manager_;
};

TEST_F(PurchasedStateManagerTest, UnresolvedStateReadsAsNotPurchased) {
  CreateManager();

  const mojom::PurchasedInfo info = manager_->GetInfo();
  EXPECT_EQ(info.state, mojom::PurchasedState::NOT_PURCHASED);
  EXPECT_EQ(info.description, std::nullopt);
  EXPECT_FALSE(manager_->IsPurchased());
}

TEST_F(PurchasedStateManagerTest, EnvironmentIsReadFromPrefs) {
  CreateManager();
  EXPECT_EQ(manager_->GetCurrentEnvironment(), skus::GetDefaultEnvironment());
  local_pref_service_.SetString(prefs::kBraveVPNEnvironment, kTestEnvironment);
  EXPECT_EQ(manager_->GetCurrentEnvironment(), kTestEnvironment);
}

}  // namespace brave_vpn::v2
