/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/v2/brave_vpn_service_impl.h"

#include <memory>
#include <optional>
#include <string>

#include "base/functional/bind.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "brave/components/brave_vpn/common/brave_vpn_utils.h"
#include "brave/components/brave_vpn/common/features.h"
#include "brave/components/brave_vpn/common/mojom/brave_vpn.mojom.h"
#include "brave/components/brave_vpn/common/pref_names.h"
#include "brave/components/skus/browser/skus_utils.h"
#include "brave/components/skus/browser/test/fake_skus_service.h"
#include "brave/components/skus/common/features.h"
#include "brave/components/skus/common/skus_sdk.mojom.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_vpn::v2 {
namespace {
constexpr char kTestDomain[] = "vpn.brave.com";
constexpr char kTestEnvironment[] = "unittest-env";
}  // namespace

class BraveVpnServiceImplTest : public testing::Test {
 public:
  BraveVpnServiceImplTest() {
    scoped_feature_list_.InitWithFeatures(
        {skus::features::kSkusFeature, features::kBraveVPN}, {});
  }

  void SetUp() override {
    brave_vpn::RegisterLocalStatePrefs(local_pref_service_.registry());
    brave_vpn::RegisterProfilePrefs(profile_pref_service_.registry());
  }

  mojo::PendingRemote<skus::mojom::SkusService> GetSkusService() {
    ++skus_bind_count_;
    return fake_skus_service_.MakeRemote();
  }

  void CreateService() {
    service_ = std::make_unique<BraveVpnServiceImpl>(
        &local_pref_service_, &profile_pref_service_,
        base::BindRepeating(&BraveVpnServiceImplTest::GetSkusService,
                            base::Unretained(this)));
  }

  void ShutdownService() { service_->Shutdown(); }

 protected:
  base::test::TaskEnvironment task_environment_;
  base::test::ScopedFeatureList scoped_feature_list_;
  TestingPrefServiceSimple local_pref_service_;
  sync_preferences::TestingPrefServiceSyncable profile_pref_service_;
  skus::FakeSkusService fake_skus_service_;
  int skus_bind_count_ = 0;
  // Declared last so it is destroyed before the prefs and the fake SKUS
  // service it points at.
  std::unique_ptr<BraveVpnServiceImpl> service_;
};

TEST_F(BraveVpnServiceImplTest, ConstructorCreatesWorkingManager) {
  CreateService();

  // The stub manager's contract: unresolved reads as not-purchased.
  EXPECT_FALSE(service_->IsPurchased());
  EXPECT_EQ(service_->GetCurrentEnvironment(), skus::GetDefaultEnvironment());
  EXPECT_EQ(skus_bind_count_, 0);
}

TEST_F(BraveVpnServiceImplTest, EnvironmentComesFromLocalPrefs) {
  CreateService();
  local_pref_service_.SetString(prefs::kBraveVPNEnvironment, kTestEnvironment);
  EXPECT_EQ(service_->GetCurrentEnvironment(), kTestEnvironment);
}

TEST_F(BraveVpnServiceImplTest, LoadPurchasedStateConnectsToSkus) {
  CreateService();
  service_->LoadPurchasedState(kTestDomain);
  EXPECT_EQ(skus_bind_count_, 1);
}

// Every entry point that delegates to the manager must return a safe default
// (not crash) after KeyedService::Shutdown destroys it. This is the test that
// guards the null-checks in the service; a newly added delegating method that
// forgets its guard should extend this test.
TEST_F(BraveVpnServiceImplTest, SafeDefaultsAfterShutdown) {
  CreateService();
  ShutdownService();

  EXPECT_FALSE(service_->IsPurchased());
  EXPECT_TRUE(service_->GetCurrentEnvironment().empty());

  // Must not crash.
  service_->ReloadPurchasedState();
  service_->LoadPurchasedState(kTestDomain);
  EXPECT_EQ(skus_bind_count_, 0);

  base::test::TestFuture<mojom::PurchasedInfoPtr> future;
  service_->GetPurchasedState(future.GetCallback());
  const mojom::PurchasedInfoPtr& info = future.Get();
  ASSERT_TRUE(info);
  EXPECT_EQ(info->state, mojom::PurchasedState::NOT_PURCHASED);
  EXPECT_EQ(info->description, std::nullopt);
}

}  // namespace brave_vpn::v2
