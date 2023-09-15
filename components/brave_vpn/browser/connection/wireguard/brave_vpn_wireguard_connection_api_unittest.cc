/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/connection/wireguard/brave_vpn_wireguard_connection_api_base.h"

#include <memory>
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
class BraveVPNWireguardConnectionAPISim
    : public BraveVPNWireguardConnectionAPIBase {
 public:
  BraveVPNWireguardConnectionAPISim(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      PrefService* local_prefs)
      : BraveVPNWireguardConnectionAPIBase(url_loader_factory, local_prefs) {}

  ~BraveVPNWireguardConnectionAPISim() override {}

  BraveVPNWireguardConnectionAPISim(const BraveVPNWireguardConnectionAPISim&) =
      delete;
  BraveVPNWireguardConnectionAPISim& operator=(
      const BraveVPNWireguardConnectionAPISim&) = delete;

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
    connection_api_ = std::make_unique<BraveVPNWireguardConnectionAPISim>(
        base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
            &url_loader_factory_),
        local_state());
  }

  BraveVPNWireguardConnectionAPIBase* GetBraveVPNWireguardConnectionAPIBase()
      const {
    return static_cast<BraveVPNWireguardConnectionAPIBase*>(
        connection_api_.get());
  }
  PrefService* local_state() { return &local_pref_service_; }

  BraveVPNOSConnectionAPI* GetConnectionAPI() { return connection_api_.get(); }

 protected:
  TestingPrefServiceSimple local_pref_service_;
  network::TestURLLoaderFactory url_loader_factory_;
  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<BraveVPNOSConnectionAPI> connection_api_;
};

TEST_F(BraveVPNWireguardConnectionAPIUnitTest, SetSelectedRegion) {
  local_state()->SetString(prefs::kBraveVPNWireguardProfileCredentials,
                           "region-a");
  GetBraveVPNWireguardConnectionAPIBase()->hostname_ =
      std::make_unique<Hostname>();
  GetBraveVPNWireguardConnectionAPIBase()->hostname_->hostname = "test";
  GetBraveVPNWireguardConnectionAPIBase()->SetSelectedRegion("region-b");
  EXPECT_TRUE(local_state()
                  ->GetString(prefs::kBraveVPNWireguardProfileCredentials)
                  .empty());
  EXPECT_EQ(GetBraveVPNWireguardConnectionAPIBase()->hostname_.get(), nullptr);
}

}  // namespace brave_vpn
