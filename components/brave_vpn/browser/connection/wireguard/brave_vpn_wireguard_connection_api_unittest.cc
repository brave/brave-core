/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>

#include "brave/components/brave_vpn/browser/connection/brave_vpn_connection_manager.h"
#include "brave/components/brave_vpn/browser/connection/wireguard/wireguard_connection_api_impl_base.h"
#include "brave/components/brave_vpn/common/brave_vpn_data_types.h"
#include "brave/components/brave_vpn/common/brave_vpn_utils.h"
#include "brave/components/brave_vpn/common/pref_names.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/browser_task_environment.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_vpn {

namespace {
class WireguardConnectionAPISim : public WireguardConnectionAPIImplBase {
 public:
  WireguardConnectionAPISim(
      BraveVPNConnectionManager* manager,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
      : WireguardConnectionAPIImplBase(manager, url_loader_factory) {}
  ~WireguardConnectionAPISim() override {}

  void Disconnect() override {}
  void CheckConnection() override {}
  void PlatformConnectImpl(
      const wireguard::WireguardProfileCredentials& credentials) override {}
};
}  // namespace

class BraveVPNWireguardConnectionAPIUnitTest : public testing::Test {
 public:
  BraveVPNWireguardConnectionAPIUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  void SetUp() override {
    brave_vpn::RegisterLocalStatePrefs(local_pref_service_.registry());
    shared_url_loader_factory_ =
        base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
            &url_loader_factory_);
    connection_manager_ = std::make_unique<BraveVPNConnectionManager>(
        shared_url_loader_factory_, &local_pref_service_, base::NullCallback());
    connection_manager_->SetConnectionAPIImplForTesting(
        std::make_unique<WireguardConnectionAPISim>(
            connection_manager_.get(), shared_url_loader_factory_));
  }

  WireguardConnectionAPISim* GetConnectionAPI() {
    return static_cast<WireguardConnectionAPISim*>(
        connection_manager_->connection_api_impl_.get());
  }
  PrefService* local_state() { return &local_pref_service_; }

 protected:
  TestingPrefServiceSimple local_pref_service_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<BraveVPNConnectionManager> connection_manager_;
};

TEST_F(BraveVPNWireguardConnectionAPIUnitTest, SetSelectedRegion) {
  local_state()->SetString(prefs::kBraveVPNWireguardProfileCredentials,
                           "region-a");
  GetConnectionAPI()->hostname_ = std::make_unique<Hostname>();
  GetConnectionAPI()->hostname_->hostname = "test";
  GetConnectionAPI()->SetSelectedRegion("region-b");
  EXPECT_TRUE(local_state()
                  ->GetString(prefs::kBraveVPNWireguardProfileCredentials)
                  .empty());
  EXPECT_EQ(GetConnectionAPI()->hostname_.get(), nullptr);
}

}  // namespace brave_vpn
