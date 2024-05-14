/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/toolbar/brave_vpn_button.h"

#include <memory>
#include <utility>

#include "base/functional/callback_helpers.h"
#include "base/run_loop.h"
#include "brave/browser/brave_vpn/brave_vpn_service_factory.h"
#include "brave/components/brave_vpn/browser/brave_vpn_service.h"
#include "brave/components/brave_vpn/browser/connection/brave_vpn_connection_manager.h"
#include "brave/components/brave_vpn/browser/connection/connection_api_impl.h"
#include "brave/components/brave_vpn/browser/connection/ikev2/connection_api_impl_sim.h"
#include "brave/components/skus/browser/skus_utils.h"
#include "brave/test/base/testing_brave_browser_process.h"
#include "chrome/browser/prefs/browser_prefs.h"
#include "chrome/browser/themes/theme_service_factory.h"
#include "chrome/browser/ui/views/chrome_layout_provider.h"
#include "chrome/test/base/scoped_testing_local_state.h"
#include "chrome/test/base/test_browser_window.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/browser_task_environment.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_vpn {

class BraveVpnButtonUnitTest : public testing::Test {
 public:
  BraveVpnButtonUnitTest()
      : testing_local_state_(TestingBrowserProcess::GetGlobal()) {}

  BraveVpnButtonUnitTest(const BraveVpnButtonUnitTest&) = delete;
  BraveVpnButtonUnitTest& operator=(const BraveVpnButtonUnitTest&) = delete;

  Browser* GetBrowser() {
    if (!browser_) {
      Browser::CreateParams params(profile(), true);
      test_window_ = std::make_unique<TestBrowserWindow>();
      params.window = test_window_.get();
      browser_.reset(Browser::Create(params));
    }
    return browser_.get();
  }
  Profile* profile() { return profile_.get(); }

  void SetUp() override {
    TestingProfile::Builder builder;
    builder.AddTestingFactory(BraveVpnServiceFactory::GetInstance(),
                              BraveVpnServiceFactory::GetDefaultFactory());

    auto prefs =
        std::make_unique<sync_preferences::TestingPrefServiceSyncable>();
    RegisterUserProfilePrefs(prefs->registry());
    builder.SetPrefService(std::move(prefs));

    profile_ = builder.Build();

    shared_url_loader_factory_ =
        base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
            &url_loader_factory_);
    auto manager = std::make_unique<BraveVPNConnectionManager>(
        shared_url_loader_factory_, testing_local_state_.Get(),
        base::NullCallback());
    manager->SetConnectionAPIImplForTesting(
        std::make_unique<ConnectionAPIImplSim>(manager.get(),
                                               shared_url_loader_factory_));
    TestingBraveBrowserProcess::GetGlobal()
        ->SetBraveVPNConnectionManagerForTesting(std::move(manager));
    ASSERT_TRUE(brave_vpn::BraveVpnServiceFactory::GetForProfile(
        GetBrowser()->profile()));
    ASSERT_TRUE(ThemeServiceFactory::GetForProfile(profile()));
    button_ = std::make_unique<BraveVPNButton>(GetBrowser());
  }

  void TearDown() override {
    browser_.reset();
    profile_.reset();

    // BraveVPNConnectionManager should be reset after profile is destoryed
    // and before local_state is gone as it uses local_state.
    TestingBraveBrowserProcess::GetGlobal()
        ->SetBraveVPNConnectionManagerForTesting(nullptr);

    base::RunLoop().RunUntilIdle();
  }

  bool IsErrorState() { return button_->IsErrorState(); }

  void FireVpnState(mojom::ConnectionState state) {
    button_->SetVpnConnectionStateForTesting(state);
    button_->OnConnectionStateChanged(state);
  }

  // Give button's visual connection state.
  bool DoesButtonHaveConnectedState() const { return button_->is_connected_; }

  void SetPurchasedState(const std::string& env, mojom::PurchasedState state) {
    button_->service_->SetPurchasedState(env, state);
  }

  void SetConnectionState(mojom::ConnectionState state) {
    ASSERT_TRUE(button_->service_->connection_manager_->connection_api_impl_);
    button_->service_->connection_manager_->connection_api_impl_
        ->SetConnectionStateForTesting(state);
  }

  bool IsOsVpnConnected() const {
    return button_->service_->GetConnectionState() ==
           mojom::ConnectionState::CONNECTED;
  }

  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<BraveVPNButton> button_;
  ChromeLayoutProvider layout_provider_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  ScopedTestingLocalState testing_local_state_;
  std::unique_ptr<Browser> browser_;
  std::unique_ptr<TestBrowserWindow> test_window_;
  std::unique_ptr<TestingProfile> profile_;
};

TEST_F(BraveVpnButtonUnitTest, SkipAttemptsToConnectInFailedState) {
  EXPECT_FALSE(IsErrorState());
  FireVpnState(brave_vpn::mojom::ConnectionState::CONNECT_FAILED);
  EXPECT_TRUE(IsErrorState());
  FireVpnState(brave_vpn::mojom::ConnectionState::CONNECTING);
  EXPECT_TRUE(IsErrorState());
  FireVpnState(brave_vpn::mojom::ConnectionState::DISCONNECTING);
  EXPECT_TRUE(IsErrorState());
  FireVpnState(brave_vpn::mojom::ConnectionState::CONNECTED);
  EXPECT_FALSE(IsErrorState());

  FireVpnState(brave_vpn::mojom::ConnectionState::CONNECT_FAILED);
  EXPECT_TRUE(IsErrorState());
  FireVpnState(brave_vpn::mojom::ConnectionState::DISCONNECTED);
  EXPECT_FALSE(IsErrorState());
}

TEST_F(BraveVpnButtonUnitTest, ButtonStateTestWithPurchasedState) {
  // Set underlying vpn state as connected state and check button's connect
  // state.
  std::string env = skus::GetDefaultEnvironment();
  SetConnectionState(mojom::ConnectionState::CONNECTED);
  EXPECT_TRUE(IsOsVpnConnected());

  // Button should have not connected state when not purchased.
  SetPurchasedState(env, mojom::PurchasedState::NOT_PURCHASED);
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(DoesButtonHaveConnectedState());
  EXPECT_TRUE(IsOsVpnConnected());

  // Button should have connected state when changed to purchased.
  SetPurchasedState(env, mojom::PurchasedState::PURCHASED);
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(DoesButtonHaveConnectedState());
  EXPECT_TRUE(IsOsVpnConnected());

  SetPurchasedState(env, mojom::PurchasedState::NOT_PURCHASED);
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(DoesButtonHaveConnectedState());
  EXPECT_TRUE(IsOsVpnConnected());
}

}  // namespace brave_vpn
