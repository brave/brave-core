/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/brave_wallet_provider_impl.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/values.h"
#include "brave/browser/brave_wallet/brave_wallet_provider_delegate_impl.h"
#include "brave/browser/brave_wallet/eth_tx_controller_factory.h"
#include "brave/browser/brave_wallet/keyring_controller_factory.h"
#include "brave/browser/brave_wallet/rpc_controller_factory.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/eth_json_rpc_controller.h"
#include "brave/components/brave_wallet/browser/eth_tx_controller.h"
#include "brave/components/brave_wallet/browser/ethereum_permission_utils.h"
#include "brave/components/brave_wallet/browser/hd_keyring.h"
#include "brave/components/brave_wallet/browser/keyring_controller.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/web3_provider_constants.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/prefs/browser_prefs.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/test/base/testing_profile.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/prefs/pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_web_contents_factory.h"
#include "content/test/test_web_contents.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

namespace {

void ValidateErrorCode(BraveWalletProviderImpl* provider,
                       const std::string& payload,
                       ProviderErrors expected) {
  bool callback_is_called = false;
  provider->AddEthereumChain(
      payload,
      base::BindLambdaForTesting(
          [&callback_is_called, &expected](bool success, int error_code,
                                           const std::string& error_message) {
            ASSERT_FALSE(success);
            EXPECT_EQ(error_code, static_cast<int>(expected));
            ASSERT_FALSE(error_message.empty());
            callback_is_called = true;
          }));
  ASSERT_TRUE(callback_is_called);
}

}  // namespace

class BraveWalletProviderImplUnitTest : public testing::Test {
 public:
  BraveWalletProviderImplUnitTest() = default;

  void TearDown() override {
    provider_.reset();
    web_contents_.reset();
  }

  void SetUp() override {
    web_contents_ =
        content::TestWebContents::Create(browser_context(), nullptr);
    eth_json_rpc_controller_ =
        brave_wallet::RpcControllerFactory::GetControllerForContext(
            browser_context());
    keyring_controller_ =
        KeyringControllerFactory::GetControllerForContext(browser_context());
    eth_tx_controller_ =
        brave_wallet::EthTxControllerFactory::GetControllerForContext(
            browser_context());
    eth_json_rpc_controller_->SetNetwork("0x1");
    base::RunLoop().RunUntilIdle();
    provider_ = std::make_unique<BraveWalletProviderImpl>(
        eth_json_rpc_controller()->MakeRemote(),
        eth_tx_controller()->MakeRemote(),
        std::make_unique<brave_wallet::BraveWalletProviderDelegateImpl>(
            web_contents(), web_contents()->GetMainFrame()),
        prefs());
  }

  ~BraveWalletProviderImplUnitTest() override = default;

  content::TestWebContents* web_contents() { return web_contents_.get(); }
  EthTxController* eth_tx_controller() { return eth_tx_controller_; }
  EthJsonRpcController* eth_json_rpc_controller() {
    return eth_json_rpc_controller_;
  }
  KeyringController* keyring_controller() { return keyring_controller_; }
  BraveWalletProviderImpl* provider() { return provider_.get(); }
  std::string from() {
    return keyring_controller()->default_keyring_->GetAddress(0);
  }

  content::BrowserContext* browser_context() { return &profile_; }
  PrefService* prefs() { return profile_.GetPrefs(); }
  HostContentSettingsMap* host_content_settings_map() {
    return HostContentSettingsMapFactory::GetForProfile(&profile_);
  }
  void CreateWalletAndAccount() {
    keyring_controller()->CreateWallet(
        "testing123", base::DoNothing::Once<const std::string&>());
    base::RunLoop().RunUntilIdle();
    keyring_controller()->AddAccount("Account 1",
                                     base::DoNothing::Once<bool>());
    base::RunLoop().RunUntilIdle();
  }

  void NavigateAndAddEthereumPermission() {
    GURL url("https://brave.com");
    web_contents()->NavigateAndCommit(url);
    GURL sub_request_origin;
    ASSERT_TRUE(
        brave_wallet::GetSubRequestOrigin(url, from(), &sub_request_origin));
    host_content_settings_map()->SetContentSettingDefaultScope(
        sub_request_origin, url, ContentSettingsType::BRAVE_ETHEREUM,
        ContentSetting::CONTENT_SETTING_ALLOW);
  }

 protected:
  content::BrowserTaskEnvironment browser_task_environment_;

 private:
  EthJsonRpcController* eth_json_rpc_controller_;
  KeyringController* keyring_controller_;
  EthTxController* eth_tx_controller_;
  content::TestWebContentsFactory factory_;
  std::unique_ptr<content::TestWebContents> web_contents_;
  std::unique_ptr<BraveWalletProviderImpl> provider_;
  network::TestURLLoaderFactory url_loader_factory_;
  base::ScopedTempDir temp_dir_;
  TestingProfile profile_;
};

TEST_F(BraveWalletProviderImplUnitTest, ValidateBrokenPayloads) {
  ValidateErrorCode(provider(), "", ProviderErrors::kInvalidParams);
  ValidateErrorCode(provider(), R"({})", ProviderErrors::kInvalidParams);
  ValidateErrorCode(provider(), R"({"params": []})",
                    ProviderErrors::kInvalidParams);
  ValidateErrorCode(provider(), R"({"params": [{}]})",
                    ProviderErrors::kInvalidParams);
  ValidateErrorCode(provider(), R"({"params": {}})",
                    ProviderErrors::kInvalidParams);
  ValidateErrorCode(provider(), R"({"params": [{
        "chainName": 'Binance1 Smart Chain',
      }]})",
                    ProviderErrors::kInvalidParams);
  ValidateErrorCode(provider(), R"({"params": [{
      "chainId": '0x386'
    }]})",
                    ProviderErrors::kInvalidParams);
  ValidateErrorCode(provider(), R"({"params": [{
      "rpcUrls": ['https://bsc-dataseed.binance.org/'],
    }]})",
                    ProviderErrors::kInvalidParams);
  ValidateErrorCode(provider(), R"({"params": [{
      "chainName": 'Binance1 Smart Chain',
      "rpcUrls": ['https://bsc-dataseed.binance.org/'],
    }]})",
                    ProviderErrors::kInvalidParams);
}

TEST_F(BraveWalletProviderImplUnitTest, EmptyDelegate) {
  BraveWalletProviderImpl provider_impl(eth_json_rpc_controller()->MakeRemote(),
                                        eth_tx_controller()->MakeRemote(),
                                        nullptr, prefs());
  ValidateErrorCode(&provider_impl,
                    R"({"params": [{
        "chainId": "0x111",
        "chainName": "Binance1 Smart Chain",
        "rpcUrls": ["https://bsc-dataseed.binance.org/"]
      }]})",
                    ProviderErrors::kInternalError);
}

TEST_F(BraveWalletProviderImplUnitTest, OnAddEthereumChain) {
  bool callback_is_called = false;
  provider()->AddEthereumChain(
      R"({"params": [{
        "chainId": "0x111",
        "chainName": "Binance1 Smart Chain",
        "rpcUrls": ["https://bsc-dataseed.binance.org/"]
      }]})",
      base::BindLambdaForTesting(
          [&callback_is_called](bool success, int error_code,
                                const std::string& error_message) {
            ASSERT_FALSE(success);
            EXPECT_EQ(error_code,
                      static_cast<int>(ProviderErrors::kUserRejectedRequest));
            ASSERT_FALSE(error_message.empty());
            callback_is_called = true;
          }));
  ASSERT_FALSE(callback_is_called);
  provider()->OnAddEthereumChain("0x111", false);
  ASSERT_TRUE(callback_is_called);
}

TEST_F(BraveWalletProviderImplUnitTest,
       OnAddEthereumChainRequestCompletedError) {
  int callback_is_called = 0;
  provider()->AddEthereumChain(
      R"({"params": [{
        "chainId": "0x111",
        "chainName": "Binance1 Smart Chain",
        "rpcUrls": ["https://bsc-dataseed.binance.org/"]
      }]})",
      base::BindLambdaForTesting(
          [&callback_is_called](bool success, int error_code,
                                const std::string& error_message) {
            ASSERT_FALSE(success);
            EXPECT_EQ(error_code,
                      static_cast<int>(ProviderErrors::kUserRejectedRequest));
            EXPECT_EQ(error_message, "test message");
            callback_is_called++;
          }));
  EXPECT_EQ(callback_is_called, 0);
  provider()->OnAddEthereumChainRequestCompleted("0x111", "test message");
  EXPECT_EQ(callback_is_called, 1);
  provider()->OnAddEthereumChainRequestCompleted("0x111", "test message");
  EXPECT_EQ(callback_is_called, 1);
}

TEST_F(BraveWalletProviderImplUnitTest, AddUnapprovedTransaction) {
  // This makes sure the state manager gets the chain ID
  browser_task_environment_.RunUntilIdle();
  bool callback_called = false;
  std::string tx_meta_id;
  CreateWalletAndAccount();
  NavigateAndAddEthereumPermission();
  provider()->AddUnapprovedTransaction(
      mojom::TxData::New("0x06", "0x09184e72a000", "0x0974",
                         "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c",
                         "0x016345785d8a0000", std::vector<uint8_t>()),
      from(),
      base::BindLambdaForTesting([&](bool success, const std::string& id,
                                     const std::string& error_message) {
        EXPECT_TRUE(success);
        EXPECT_FALSE(id.empty());
        EXPECT_TRUE(error_message.empty());
        callback_called = true;
        tx_meta_id = id;
      }));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);

  // Make sure the transaction info is available
  callback_called = false;
  eth_tx_controller()->GetAllTransactionInfo(
      from(),
      base::BindLambdaForTesting([&](std::vector<mojom::TransactionInfoPtr> v) {
        ASSERT_EQ(v.size(), 1UL);
        EXPECT_TRUE(
            base::EqualsCaseInsensitiveASCII(v[0]->from_address, from()));
        EXPECT_EQ(v[0]->tx_status, mojom::TransactionStatus::Unapproved);
        EXPECT_EQ(v[0]->id, tx_meta_id);
        callback_called = true;
      }));
  browser_task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_called);
}

TEST_F(BraveWalletProviderImplUnitTest, AddUnapprovedTransactionError) {
  // This makes sure the state manager gets the chain ID
  browser_task_environment_.RunUntilIdle();

  // We don't need to check every error type since that is checked by
  // eth_tx_controller_unittest but make sure an error type is handled
  // correctly.
  bool callback_called = false;
  CreateWalletAndAccount();
  NavigateAndAddEthereumPermission();
  provider()->AddUnapprovedTransaction(
      mojom::TxData::New("0x06", "0x09184e72a000", "0x0974",
                         // Bad address
                         "0xbe8", "0x016345785d8a0000", std::vector<uint8_t>()),
      from(),
      base::BindLambdaForTesting([&](bool success, const std::string& id,
                                     const std::string& error_message) {
        EXPECT_FALSE(success);
        EXPECT_TRUE(id.empty());
        EXPECT_FALSE(error_message.empty());
        callback_called = true;
      }));
  browser_task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_called);
}

TEST_F(BraveWalletProviderImplUnitTest, AddUnapprovedTransactionNoPermission) {
  // This makes sure the state manager gets the chain ID
  browser_task_environment_.RunUntilIdle();

  bool callback_called = false;
  provider()->AddUnapprovedTransaction(
      mojom::TxData::New("0x06", "0x09184e72a000", "0x0974",
                         "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c",
                         "0x016345785d8a0000", std::vector<uint8_t>()),
      "0xbe862ad9abfe6f22bcb087716c7d89a26051f74d",
      base::BindLambdaForTesting([&](bool success, const std::string& id,
                                     const std::string& error_message) {
        EXPECT_FALSE(success);
        EXPECT_TRUE(id.empty());
        EXPECT_FALSE(error_message.empty());
        callback_called = true;
      }));
  browser_task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_called);
}

TEST_F(BraveWalletProviderImplUnitTest, AddUnapproved1559Transaction) {
  // This makes sure the state manager gets the chain ID
  browser_task_environment_.RunUntilIdle();
  bool callback_called = false;
  std::string tx_meta_id;
  CreateWalletAndAccount();
  NavigateAndAddEthereumPermission();
  provider()->AddUnapproved1559Transaction(
      mojom::TxData1559::New(
          mojom::TxData::New("0x00", "", "0x00",
                             "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c",
                             "0x00", std::vector<uint8_t>()),
          "0x04", "0x0", "0x0"),
      from(),
      base::BindLambdaForTesting([&](bool success, const std::string& id,
                                     const std::string& error_message) {
        EXPECT_TRUE(success);
        EXPECT_FALSE(id.empty());
        EXPECT_TRUE(error_message.empty());
        callback_called = true;
        tx_meta_id = id;
      }));
  browser_task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_called);

  // Make sure the transaction info is available
  callback_called = false;
  eth_tx_controller()->GetAllTransactionInfo(
      from(),
      base::BindLambdaForTesting([&](std::vector<mojom::TransactionInfoPtr> v) {
        ASSERT_EQ(v.size(), 1UL);
        EXPECT_TRUE(
            base::EqualsCaseInsensitiveASCII(v[0]->from_address, from()));
        EXPECT_EQ(v[0]->tx_status, mojom::TransactionStatus::Unapproved);
        EXPECT_EQ(v[0]->id, tx_meta_id);
        callback_called = true;
      }));
  browser_task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_called);
}

TEST_F(BraveWalletProviderImplUnitTest, AddUnapproved1559TransactionError) {
  // This makes sure the state manager gets the chain ID
  browser_task_environment_.RunUntilIdle();

  // We don't need to check every error type since that is checked by
  // eth_tx_controller_unittest but make sure an error type is handled
  // correctly.
  bool callback_called = false;
  CreateWalletAndAccount();
  NavigateAndAddEthereumPermission();
  provider()->AddUnapproved1559Transaction(
      mojom::TxData1559::New(
          // Can't specify both gas price and max fee per gas
          mojom::TxData::New("0x00", "0x01", "0x00",
                             "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c",
                             "0x00", std::vector<uint8_t>()),
          "0x04", "0x0", "0x0"),
      from(),
      base::BindLambdaForTesting([&](bool success, const std::string& id,
                                     const std::string& error_message) {
        EXPECT_FALSE(success);
        EXPECT_TRUE(id.empty());
        EXPECT_FALSE(error_message.empty());
        callback_called = true;
      }));
  browser_task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_called);
}

TEST_F(BraveWalletProviderImplUnitTest,
       AddUnapproved1559TransactionNoPermission) {
  // This makes sure the state manager gets the chain ID
  browser_task_environment_.RunUntilIdle();

  bool callback_called = false;
  provider()->AddUnapproved1559Transaction(
      mojom::TxData1559::New(
          mojom::TxData::New("0x00", "", "0x00",
                             "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c",
                             "0x00", std::vector<uint8_t>()),
          "0x04", "0x0", "0x0"),
      "0xbe862ad9abfe6f22bcb087716c7d89a26051f74d",
      base::BindLambdaForTesting([&](bool success, const std::string& id,
                                     const std::string& error_message) {
        EXPECT_FALSE(success);
        EXPECT_TRUE(id.empty());
        EXPECT_FALSE(error_message.empty());
        callback_called = true;
      }));
  browser_task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_called);
}

TEST_F(BraveWalletProviderImplUnitTest, RequestEthereumPermissions) {
  bool new_setup_callback_called = false;
  BraveWalletProviderDelegateImpl::SetCallbackForNewSetupNeededForTesting(
      base::BindLambdaForTesting([&]() { new_setup_callback_called = true; }));
  CreateWalletAndAccount();
  NavigateAndAddEthereumPermission();
  bool permission_callback_called = false;
  provider()->RequestEthereumPermissions(base::BindLambdaForTesting(
      [&](bool success, const std::vector<std::string>& allowed_accounts) {
        EXPECT_TRUE(success);
        EXPECT_EQ(allowed_accounts.size(), 1UL);
        EXPECT_EQ(allowed_accounts[0], from());
        permission_callback_called = true;
      }));
  browser_task_environment_.RunUntilIdle();
  EXPECT_TRUE(permission_callback_called);
  EXPECT_FALSE(new_setup_callback_called);
}

TEST_F(BraveWalletProviderImplUnitTest,
       RequestEthereumPermissionsNoPermission) {
  bool new_setup_callback_called = false;
  BraveWalletProviderDelegateImpl::SetCallbackForNewSetupNeededForTesting(
      base::BindLambdaForTesting([&]() { new_setup_callback_called = true; }));
  bool permission_callback_called = false;
  CreateWalletAndAccount();
  provider()->RequestEthereumPermissions(base::BindLambdaForTesting(
      [&](bool success, const std::vector<std::string>& allowed_accounts) {
        EXPECT_FALSE(success);
        EXPECT_TRUE(allowed_accounts.empty());
        permission_callback_called = true;
      }));
  browser_task_environment_.RunUntilIdle();
  EXPECT_TRUE(permission_callback_called);
  EXPECT_FALSE(new_setup_callback_called);
}

TEST_F(BraveWalletProviderImplUnitTest, RequestEthereumPermissionsNoWallet) {
  bool new_setup_callback_called = false;
  BraveWalletProviderDelegateImpl::SetCallbackForNewSetupNeededForTesting(
      base::BindLambdaForTesting([&]() { new_setup_callback_called = true; }));
  bool permission_callback_called = false;
  provider()->RequestEthereumPermissions(base::BindLambdaForTesting(
      [&](bool success, const std::vector<std::string>& allowed_accounts) {
        EXPECT_FALSE(success);
        EXPECT_TRUE(allowed_accounts.empty());
        permission_callback_called = true;
      }));
  browser_task_environment_.RunUntilIdle();
  EXPECT_TRUE(permission_callback_called);
  EXPECT_TRUE(new_setup_callback_called);
}

}  // namespace brave_wallet
