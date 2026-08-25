/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/v2/brave_vpn_service_impl.h"

#include <memory>
#include <optional>
#include <string>

#include "base/functional/bind.h"
#include "base/task/thread_pool/thread_pool_instance.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/values.h"
#include "brave/components/brave_vpn/common/brave_vpn_utils.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/brave_vpn/common/features.h"
#include "brave/components/brave_vpn/common/mojom/brave_vpn.mojom.h"
#include "brave/components/brave_vpn/common/pref_names.h"
#include "brave/components/skus/browser/skus_utils.h"
#include "brave/components/skus/browser/test/fake_skus_service.h"
#include "brave/components/skus/common/features.h"
#include "brave/components/skus/common/skus_sdk.mojom.h"
#include "build/build_config.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

#if BUILDFLAG(ENABLE_BRAVE_VPN_V2_APPS)
#include "brave/components/brave_vpn/browser/v2/test/fake_agent.h"
#endif  // BUILDFLAG(ENABLE_BRAVE_VPN_V2_APPS)

namespace brave_vpn::v2 {
namespace {
constexpr char kTestDomain[] = "vpn.brave.com";
constexpr char kTestEnvironment[] = "unittest-env";
#if !BUILDFLAG(IS_ANDROID)
constexpr char kTestEmail[] = "test@example.com";
#endif  // !BUILDFLAG(IS_ANDROID)
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

  void TearDown() override {
    base::ThreadPoolInstance::Get()->FlushForTesting();
  }

  mojo::PendingRemote<skus::mojom::SkusService> GetSkusService() {
    ++skus_bind_count_;
    return fake_skus_service_.MakeRemote();
  }

  void CreateService() {
    service_ = std::make_unique<BraveVpnServiceImpl>(
        &local_pref_service_, &profile_pref_service_,
        url_loader_factory_.GetSafeWeakWrapper(),
        base::BindRepeating(&BraveVpnServiceImplTest::GetSkusService,
                            base::Unretained(this)));
#if BUILDFLAG(ENABLE_BRAVE_VPN_V2_APPS)
    // Replace the inert agent client the constructor made with one that never
    // talks to the real agent, so tests are properly isolated.
    ASSERT_TRUE(service_->agent_client_);
    ASSERT_EQ(service_->agent_client_->state(),
              AgentClient::State::kDisconnected);
    ReplaceAgentClientWithFake();
#endif  // BUILDFLAG(ENABLE_BRAVE_VPN_V2_APPS)
  }

  void ShutdownService() { service_->Shutdown(); }

  void BlockVPNByPolicy(bool value) {
    profile_pref_service_.SetManagedPref(prefs::kManagedBraveVPNDisabled,
                                         base::Value(value));
    EXPECT_EQ(brave_vpn::IsBraveVPNDisabledByPolicy(&profile_pref_service_),
              value);
  }

#if BUILDFLAG(ENABLE_BRAVE_VPN_V2_APPS)
  // Mirrors the wiring in BraveVpnServiceImpl's constructor. Keep in sync.
  void ReplaceAgentClientWithFake() {
    service_->agent_client_->RemoveObserver(service_.get());
    service_->agent_client_ = AgentClient::CreateForTesting(
        fake_agent_.GetServerNameProvider(), fake_agent_.GetConnector(),
        /*tick_clock=*/nullptr);
    service_->agent_client_->AddObserver(service_.get());
  }

  void UpdateAgentConnection(mojom::PurchasedState state) {
    service_->UpdateAgentConnection(state);
  }

  AgentClient* agent_client() { return service_->agent_client_.get(); }
#endif  // BUILDFLAG(ENABLE_BRAVE_VPN_V2_APPS)

 protected:
  base::test::TaskEnvironment task_environment_;
  base::test::ScopedFeatureList scoped_feature_list_;
  TestingPrefServiceSimple local_pref_service_;
  sync_preferences::TestingPrefServiceSyncable profile_pref_service_;
  network::TestURLLoaderFactory url_loader_factory_;
  skus::FakeSkusService fake_skus_service_;
  int skus_bind_count_ = 0;
#if BUILDFLAG(ENABLE_BRAVE_VPN_V2_APPS)
  FakeAgent fake_agent_;
#endif  // BUILDFLAG(ENABLE_BRAVE_VPN_V2_APPS)
  // Declared last so it is destroyed before the prefs, the fake SKUS
  // service, and the fake agent it points at.
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
  {
    base::test::TestFuture<mojom::PurchasedInfoPtr> future;
    service_->GetPurchasedState(future.GetCallback());
    const mojom::PurchasedInfoPtr& info = future.Get();
    ASSERT_TRUE(info);
    EXPECT_EQ(info->state, mojom::PurchasedState::NOT_PURCHASED);
    EXPECT_EQ(info->description, std::nullopt);
  }

#if BUILDFLAG(ENABLE_BRAVE_VPN_V2_APPS)
  // The agent client is gone; the mapping must not dereference it.
  UpdateAgentConnection(mojom::PurchasedState::PURCHASED);
  UpdateAgentConnection(mojom::PurchasedState::NOT_PURCHASED);
#endif  // BUILDFLAG(ENABLE_BRAVE_VPN_V2_APPS)

#if !BUILDFLAG(IS_ANDROID)
  {
    base::test::TestFuture<bool, std::string> future;
    service_->CreateSupportTicket(
        kTestEmail, "subject", "body",
        future.GetCallback<bool, const std::string&>());
    EXPECT_FALSE(future.Get<0>());
    EXPECT_TRUE(future.Get<1>().empty());
  }
#endif  // !BUILDFLAG(IS_ANDROID)
}

#if BUILDFLAG(ENABLE_BRAVE_VPN_V2_APPS)

TEST_F(BraveVpnServiceImplTest, PurchasedStateDrivesAgentConnection) {
  CreateService();
  ASSERT_TRUE(agent_client());
  ASSERT_EQ(agent_client()->state(), AgentClient::State::kDisconnected);

  UpdateAgentConnection(mojom::PurchasedState::PURCHASED);
  EXPECT_EQ(agent_client()->state(), AgentClient::State::kConnecting);

  // The recoverable states leave a live connection alone: each can coexist with
  // a running tunnel, and the person still has to be able to disconnect it.
  for (const mojom::PurchasedState state :
       {mojom::PurchasedState::LOADING, mojom::PurchasedState::SESSION_EXPIRED,
        mojom::PurchasedState::FAILED,
        mojom::PurchasedState::OUT_OF_CREDENTIALS}) {
    UpdateAgentConnection(state);
    EXPECT_EQ(agent_client()->state(), AgentClient::State::kConnecting)
        << "dropped the connection on " << static_cast<int>(state);
  }

  UpdateAgentConnection(mojom::PurchasedState::NOT_PURCHASED);
  EXPECT_EQ(agent_client()->state(), AgentClient::State::kDisconnected);
}

TEST_F(BraveVpnServiceImplTest, PolicyDisabledKeepsAgentDisconnected) {
  BlockVPNByPolicy(true);
  CreateService();
  UpdateAgentConnection(mojom::PurchasedState::PURCHASED);
  EXPECT_EQ(agent_client()->state(), AgentClient::State::kDisconnected);
}

#endif  // BUILDFLAG(ENABLE_BRAVE_VPN_V2_APPS)

}  // namespace brave_vpn::v2
