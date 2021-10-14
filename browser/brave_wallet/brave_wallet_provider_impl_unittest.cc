/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/brave_wallet_provider_impl.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/callback_helpers.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/values.h"
#include "brave/browser/brave_wallet/brave_wallet_provider_delegate_impl.h"
#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/browser/brave_wallet/eth_tx_controller_factory.h"
#include "brave/browser/brave_wallet/keyring_controller_factory.h"
#include "brave/browser/brave_wallet/rpc_controller_factory.h"
#include "brave/components/brave_wallet/browser/asset_ratio_controller.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/eth_json_rpc_controller.h"
#include "brave/components/brave_wallet/browser/eth_nonce_tracker.h"
#include "brave/components/brave_wallet/browser/eth_pending_tx_tracker.h"
#include "brave/components/brave_wallet/browser/eth_tx_controller.h"
#include "brave/components/brave_wallet/browser/eth_tx_state_manager.h"
#include "brave/components/brave_wallet/browser/ethereum_permission_utils.h"
#include "brave/components/brave_wallet/browser/hd_keyring.h"
#include "brave/components/brave_wallet/browser/keyring_controller.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/web3_provider_constants.h"
#include "brave/components/permissions/contexts/brave_ethereum_permission_context.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/prefs/browser_prefs.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/test/base/testing_profile.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/grit/brave_components_strings.h"
#include "components/prefs/pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_web_contents_factory.h"
#include "content/test/test_web_contents.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "ui/base/l10n/l10n_util.h"

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

class TestEventsListener : public brave_wallet::mojom::EventsListener {
 public:
  TestEventsListener() {}

  void ChainChangedEvent(const std::string& chain_id) override {
    chain_id_ = chain_id;
    chain_changed_fired_ = true;
  }

  void AccountsChangedEvent(const std::vector<std::string>& accounts) override {
    accounts_ = accounts;
    accounts_changed_fired_ = true;
  }

  bool ChainChangedFired() const {
    base::RunLoop().RunUntilIdle();
    return chain_changed_fired_;
  }

  bool AccountsChangedFired() const {
    base::RunLoop().RunUntilIdle();
    return accounts_changed_fired_;
  }

  std::string GetChainId() const {
    base::RunLoop().RunUntilIdle();
    return chain_id_;
  }

  std::vector<std::string> GetAccounts() const {
    base::RunLoop().RunUntilIdle();
    return accounts_;
  }

  mojo::PendingRemote<brave_wallet::mojom::EventsListener> GetReceiver() {
    return observer_receiver_.BindNewPipeAndPassRemote();
  }

  void Reset() {
    chain_id_.clear();
    accounts_.clear();
    chain_changed_fired_ = false;
    accounts_changed_fired_ = false;
    EXPECT_FALSE(ChainChangedFired());
    EXPECT_FALSE(AccountsChangedFired());
  }

  bool chain_changed_fired_ = false;
  bool accounts_changed_fired_ = false;
  std::vector<std::string> accounts_;
  std::string chain_id_;

 private:
  mojo::Receiver<brave_wallet::mojom::EventsListener> observer_receiver_{this};
};

class BraveWalletProviderImplUnitTest : public testing::Test {
 public:
  BraveWalletProviderImplUnitTest()
      : shared_url_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &url_loader_factory_)) {}

  void TearDown() override {
    provider_.reset();
    web_contents_.reset();
  }

  void SetUp() override {
    web_contents_ =
        content::TestWebContents::Create(browser_context(), nullptr);
    eth_json_rpc_controller_.reset(
        new EthJsonRpcController(shared_url_loader_factory_, prefs()));
    keyring_controller_ =
        KeyringControllerFactory::GetControllerForContext(browser_context());
    asset_ratio_controller_.reset(
        new AssetRatioController(shared_url_loader_factory_));
    auto tx_state_manager = std::make_unique<EthTxStateManager>(
        prefs(), eth_json_rpc_controller()->MakeRemote());
    auto nonce_tracker = std::make_unique<EthNonceTracker>(
        tx_state_manager.get(), eth_json_rpc_controller());
    auto pending_tx_tracker = std::make_unique<EthPendingTxTracker>(
        tx_state_manager.get(), eth_json_rpc_controller(), nonce_tracker.get());
    eth_tx_controller_.reset(new EthTxController(
        eth_json_rpc_controller(), keyring_controller(),
        asset_ratio_controller_.get(), std::move(tx_state_manager),
        std::move(nonce_tracker), std::move(pending_tx_tracker), prefs()));
    brave_wallet_service_ =
        brave_wallet::BraveWalletServiceFactory::GetServiceForContext(
            browser_context());

    provider_ = std::make_unique<BraveWalletProviderImpl>(
        host_content_settings_map(), eth_json_rpc_controller()->MakeRemote(),
        eth_tx_controller()->MakeRemote(), keyring_controller_,
        brave_wallet_service_,
        std::make_unique<brave_wallet::BraveWalletProviderDelegateImpl>(
            web_contents(), web_contents()->GetMainFrame()),
        prefs());

    observer_.reset(new TestEventsListener());
    provider_->Init(observer_->GetReceiver());
    SetNetwork("0x1");
  }

  void SetInterceptor(const std::string& content) {
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&, content](const network::ResourceRequest& request) {
          url_loader_factory_.ClearResponses();
          url_loader_factory_.AddResponse(request.url.spec(), content);
        }));
  }

  void SetNetwork(const std::string& chain_id) {
    base::RunLoop run_loop;
    eth_json_rpc_controller_->SetNetwork(
        chain_id, base::BindLambdaForTesting([&run_loop](bool success) {
          EXPECT_TRUE(success);
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  void CreateWallet() {
    base::RunLoop run_loop;
    keyring_controller_->CreateWallet(
        "brave",
        base::BindLambdaForTesting([&run_loop](const std::string& mnemonic) {
          EXPECT_FALSE(mnemonic.empty());
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  void AddAccount() {
    base::RunLoop run_loop;
    keyring_controller_->AddAccount(
        "New Account", base::BindLambdaForTesting([&run_loop](bool success) {
          EXPECT_TRUE(success);
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  void SetSelectedAccount(const std::string& address) {
    base::RunLoop run_loop;
    keyring_controller_->SetSelectedAccount(
        address, base::BindLambdaForTesting([&](bool success) {
          EXPECT_TRUE(success);
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  ~BraveWalletProviderImplUnitTest() override = default;

  content::TestWebContents* web_contents() { return web_contents_.get(); }
  EthTxController* eth_tx_controller() { return eth_tx_controller_.get(); }
  EthJsonRpcController* eth_json_rpc_controller() {
    return eth_json_rpc_controller_.get();
  }
  KeyringController* keyring_controller() { return keyring_controller_; }
  BraveWalletProviderImpl* provider() { return provider_.get(); }
  std::string from(size_t from_index = 0) {
    return keyring_controller()->default_keyring_->GetAddress(from_index);
  }

  content::BrowserContext* browser_context() { return &profile_; }
  PrefService* prefs() { return profile_.GetPrefs(); }
  HostContentSettingsMap* host_content_settings_map() {
    return HostContentSettingsMapFactory::GetForProfile(&profile_);
  }

  void Navigate(const GURL& url) { web_contents()->NavigateAndCommit(url); }

  void AddEthereumPermission(const GURL& url, size_t from_index = 0) {
    GURL sub_request_origin;
    ASSERT_TRUE(brave_wallet::GetSubRequestOrigin(url, from(from_index),
                                                  &sub_request_origin));
    host_content_settings_map()->SetContentSettingDefaultScope(
        sub_request_origin, url, ContentSettingsType::BRAVE_ETHEREUM,
        ContentSetting::CONTENT_SETTING_ALLOW);
  }

  void ResetEthereumPermission(const GURL& url, size_t from_index = 0) {
    permissions::BraveEthereumPermissionContext::ResetEthereumPermission(
        browser_context(), url.spec(), from(from_index));
  }

  void SignMessage(absl::optional<bool> user_approved,
                   const std::string& address,
                   const std::string& message,
                   std::string* signature_out,
                   int* error_out,
                   std::string* error_message_out) {
    if (!signature_out || !error_out || !error_message_out)
      return;

    base::RunLoop run_loop;
    provider()->SignMessage(
        address, message,
        base::BindLambdaForTesting([&](const std::string& signature, int error,
                                       const std::string& error_message) {
          *signature_out = signature;
          *error_out = error;
          *error_message_out = error_message;
          run_loop.Quit();
        }));
    // Wait for BraveWalletProviderImpl::ContinueSignMessage
    browser_task_environment_.RunUntilIdle();
    if (user_approved)
      brave_wallet_service_->NotifySignMessageRequestProcessed(
          *user_approved, provider()->sign_message_id_ - 1);
    run_loop.Run();
  }

  // current request id will be returned
  int SignMessageRequest(const std::string& address,
                         const std::string& message) {
    provider()->SignMessage(address, message, base::DoNothing());
    base::RunLoop().RunUntilIdle();
    return provider()->sign_message_id_ - 1;
  }

  size_t GetSignMessageQueueSize() const {
    size_t request_queue_size =
        brave_wallet_service_->sign_message_requests_.size();
    EXPECT_EQ(brave_wallet_service_->sign_message_callbacks_.size(),
              request_queue_size);
    return request_queue_size;
  }

  BraveWalletService::SignMessageRequest GetSignMessageQueueFront() const {
    return brave_wallet_service_->sign_message_requests_.front();
  }

  std::vector<std::string> GetAddresses() {
    std::vector<std::string> result;
    base::RunLoop run_loop;
    keyring_controller_->GetDefaultKeyringInfo(
        base::BindLambdaForTesting([&](mojom::KeyringInfoPtr keyring_info) {
          for (size_t i = 0; i < keyring_info->account_infos.size(); ++i) {
            result.push_back(keyring_info->account_infos[i]->address);
          }
          run_loop.Quit();
        }));
    run_loop.Run();
    return result;
  }

  std::vector<mojom::TransactionInfoPtr> GetAllTransactionInfo() {
    std::vector<mojom::TransactionInfoPtr> transaction_infos;
    base::RunLoop run_loop;
    eth_tx_controller()->GetAllTransactionInfo(
        from(), base::BindLambdaForTesting(
                    [&](std::vector<mojom::TransactionInfoPtr> v) {
                      transaction_infos = std::move(v);
                      run_loop.Quit();
                    }));
    run_loop.Run();
    return transaction_infos;
  }

  bool ApproveTransaction(const std::string& tx_meta_id) {
    bool success;
    base::RunLoop run_loop;
    eth_tx_controller()->ApproveTransaction(
        tx_meta_id, base::BindLambdaForTesting([&](bool v) {
          success = v;
          run_loop.Quit();
        }));
    run_loop.Run();
    return success;
  }

 protected:
  content::BrowserTaskEnvironment browser_task_environment_;
  BraveWalletService* brave_wallet_service_;
  std::unique_ptr<TestEventsListener> observer_;

 private:
  std::unique_ptr<EthJsonRpcController> eth_json_rpc_controller_;
  KeyringController* keyring_controller_;
  content::TestWebContentsFactory factory_;
  std::unique_ptr<EthTxController> eth_tx_controller_;
  std::unique_ptr<AssetRatioController> asset_ratio_controller_;
  std::unique_ptr<content::TestWebContents> web_contents_;
  std::unique_ptr<BraveWalletProviderImpl> provider_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
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
  BraveWalletProviderImpl provider_impl(
      host_content_settings_map(), eth_json_rpc_controller()->MakeRemote(),
      eth_tx_controller()->MakeRemote(), keyring_controller(),
      brave_wallet_service_, nullptr, prefs());
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

TEST_F(BraveWalletProviderImplUnitTest, AddAndApproveTransaction) {
  // This makes sure the state manager gets the chain ID
  browser_task_environment_.RunUntilIdle();
  bool callback_called = false;
  std::string tx_hash;
  CreateWallet();
  AddAccount();
  GURL url("https://brave.com");
  Navigate(url);
  AddEthereumPermission(url);
  provider()->AddAndApproveTransaction(
      mojom::TxData::New("0x06", "0x09184e72a000", "0x0974",
                         "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c",
                         "0x016345785d8a0000", std::vector<uint8_t>()),
      from(),
      base::BindLambdaForTesting([&](bool success, const std::string& hash,
                                     const std::string& error_message) {
        EXPECT_TRUE(success);
        EXPECT_FALSE(hash.empty());
        EXPECT_TRUE(error_message.empty());
        callback_called = true;
        tx_hash = hash;
      }));
  base::RunLoop().RunUntilIdle();
  std::vector<mojom::TransactionInfoPtr> infos = GetAllTransactionInfo();
  ASSERT_EQ(infos.size(), 1UL);
  EXPECT_TRUE(base::EqualsCaseInsensitiveASCII(infos[0]->from_address, from()));
  EXPECT_EQ(infos[0]->tx_status, mojom::TransactionStatus::Unapproved);
  EXPECT_EQ(infos[0]->tx_hash, tx_hash);

  // Set an interceptor and just fake a common repsonse for
  // eth_getTransactionCount and eth_sendRawTransaction
  SetInterceptor("{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":\"0x0\"}");

  EXPECT_TRUE(ApproveTransaction(infos[0]->id));
  base::RunLoop().RunUntilIdle();
  // OnGetNextNonce
  base::RunLoop().RunUntilIdle();
  // OnPublishTransaction
  base::RunLoop().RunUntilIdle();

  EXPECT_TRUE(callback_called);
  infos = GetAllTransactionInfo();
  ASSERT_EQ(infos.size(), 1UL);
  EXPECT_TRUE(base::EqualsCaseInsensitiveASCII(infos[0]->from_address, from()));
  EXPECT_EQ(infos[0]->tx_status, mojom::TransactionStatus::Submitted);
  EXPECT_EQ(infos[0]->tx_hash, tx_hash);
}

TEST_F(BraveWalletProviderImplUnitTest, AddAndApproveTransactionError) {
  // This makes sure the state manager gets the chain ID
  browser_task_environment_.RunUntilIdle();

  // We don't need to check every error type since that is checked by
  // eth_tx_controller_unittest but make sure an error type is handled
  // correctly.
  bool callback_called = false;
  CreateWallet();
  AddAccount();
  GURL url("https://brave.com");
  Navigate(url);
  AddEthereumPermission(url);
  provider()->AddAndApproveTransaction(
      mojom::TxData::New("0x06", "0x09184e72a000", "0x0974",
                         // Bad address
                         "0xbe8", "0x016345785d8a0000", std::vector<uint8_t>()),
      from(),
      base::BindLambdaForTesting([&](bool success, const std::string& hash,
                                     const std::string& error_message) {
        EXPECT_FALSE(success);
        EXPECT_TRUE(hash.empty());
        EXPECT_FALSE(error_message.empty());
        callback_called = true;
      }));
  browser_task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_called);
}

TEST_F(BraveWalletProviderImplUnitTest, AddAndApproveTransactionNoPermission) {
  // This makes sure the state manager gets the chain ID
  browser_task_environment_.RunUntilIdle();

  bool callback_called = false;
  provider()->AddAndApproveTransaction(
      mojom::TxData::New("0x06", "0x09184e72a000", "0x0974",
                         "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c",
                         "0x016345785d8a0000", std::vector<uint8_t>()),
      "0xbe862ad9abfe6f22bcb087716c7d89a26051f74d",
      base::BindLambdaForTesting([&](bool success, const std::string& hash,
                                     const std::string& error_message) {
        EXPECT_FALSE(success);
        EXPECT_TRUE(hash.empty());
        EXPECT_FALSE(error_message.empty());
        callback_called = true;
      }));
  browser_task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_called);
}

TEST_F(BraveWalletProviderImplUnitTest, AddAndApprove1559Transaction) {
  // This makes sure the state manager gets the chain ID
  browser_task_environment_.RunUntilIdle();
  bool callback_called = false;
  std::string tx_hash;
  CreateWallet();
  AddAccount();
  GURL url("https://brave.com");
  Navigate(url);
  AddEthereumPermission(url);
  provider()->AddAndApprove1559Transaction(
      mojom::TxData1559::New(
          mojom::TxData::New("0x00", "", "0x1",
                             "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c",
                             "0x00", std::vector<uint8_t>()),
          "0x04", "0x1", "0x1", nullptr),
      from(),
      base::BindLambdaForTesting([&](bool success, const std::string& hash,
                                     const std::string& error_message) {
        EXPECT_TRUE(success);
        EXPECT_FALSE(hash.empty());
        EXPECT_TRUE(error_message.empty());
        callback_called = true;
        tx_hash = hash;
      }));
  browser_task_environment_.RunUntilIdle();
  std::vector<mojom::TransactionInfoPtr> infos = GetAllTransactionInfo();
  ASSERT_EQ(infos.size(), 1UL);
  EXPECT_TRUE(base::EqualsCaseInsensitiveASCII(infos[0]->from_address, from()));
  EXPECT_EQ(infos[0]->tx_status, mojom::TransactionStatus::Unapproved);
  EXPECT_EQ(infos[0]->tx_hash, tx_hash);

  // Set an interceptor and just fake a common repsonse for
  // eth_getTransactionCount and eth_sendRawTransaction
  SetInterceptor("{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":\"0x0\"}");

  EXPECT_TRUE(ApproveTransaction(infos[0]->id));
  base::RunLoop().RunUntilIdle();
  // OnGetNextNonce
  base::RunLoop().RunUntilIdle();
  // OnPublishTransaction
  base::RunLoop().RunUntilIdle();

  EXPECT_TRUE(callback_called);
  infos = GetAllTransactionInfo();
  ASSERT_EQ(infos.size(), 1UL);
  EXPECT_TRUE(base::EqualsCaseInsensitiveASCII(infos[0]->from_address, from()));
  EXPECT_EQ(infos[0]->tx_status, mojom::TransactionStatus::Submitted);
  EXPECT_EQ(infos[0]->tx_hash, tx_hash);
}

TEST_F(BraveWalletProviderImplUnitTest, AddAndApprove1559TransactionNoChainId) {
  // This makes sure the state manager gets the chain ID
  browser_task_environment_.RunUntilIdle();
  std::string tx_hash;
  CreateWallet();
  AddAccount();
  GURL url("https://brave.com");
  Navigate(url);
  SetNetwork("0x4");
  AddEthereumPermission(url);
  // Chain ID as 0x0
  provider()->AddAndApprove1559Transaction(
      mojom::TxData1559::New(
          mojom::TxData::New("0x00", "", "0x1",
                             "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c",
                             "0x00", std::vector<uint8_t>()),
          "0x0", "0x1", "0x1", nullptr),
      from(),
      base::BindLambdaForTesting([&](bool success, const std::string& hash,
                                     const std::string& error_message) {
        EXPECT_TRUE(success);
        EXPECT_FALSE(hash.empty());
        EXPECT_TRUE(error_message.empty());
        tx_hash = hash;
      }));
  browser_task_environment_.RunUntilIdle();
  // Chain ID as an empty string
  provider()->AddAndApprove1559Transaction(
      mojom::TxData1559::New(
          mojom::TxData::New("0x00", "", "0x1",
                             "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c",
                             "0x00", std::vector<uint8_t>()),
          "", "0x1", "0x1", nullptr),
      from(),
      base::BindLambdaForTesting([&](bool success, const std::string& hash,
                                     const std::string& error_message) {
        EXPECT_TRUE(success);
        EXPECT_FALSE(hash.empty());
        EXPECT_TRUE(error_message.empty());
        tx_hash = hash;
      }));
  browser_task_environment_.RunUntilIdle();

  std::vector<mojom::TransactionInfoPtr> infos = GetAllTransactionInfo();
  ASSERT_EQ(infos.size(), 2UL);
  EXPECT_EQ(infos[0]->tx_data->chain_id, "0x4");
  EXPECT_EQ(infos[1]->tx_data->chain_id, "0x4");
}

TEST_F(BraveWalletProviderImplUnitTest, AddAndApprove1559TransactionError) {
  // This makes sure the state manager gets the chain ID
  browser_task_environment_.RunUntilIdle();

  // We don't need to check every error type since that is checked by
  // eth_tx_controller_unittest but make sure an error type is handled
  // correctly.
  bool callback_called = false;
  CreateWallet();
  AddAccount();
  GURL url("https://brave.com");
  Navigate(url);
  AddEthereumPermission(url);
  provider()->AddAndApprove1559Transaction(
      mojom::TxData1559::New(
          // Can't specify both gas price and max fee per gas
          mojom::TxData::New("0x00", "0x01", "0x00",
                             "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c",
                             "0x00", std::vector<uint8_t>()),
          "0x04", "0x0", "0x0", nullptr),
      from(),
      base::BindLambdaForTesting([&](bool success, const std::string& hash,
                                     const std::string& error_message) {
        EXPECT_FALSE(success);
        EXPECT_TRUE(hash.empty());
        EXPECT_FALSE(error_message.empty());
        callback_called = true;
      }));
  browser_task_environment_.RunUntilIdle();
  EXPECT_TRUE(callback_called);
}

TEST_F(BraveWalletProviderImplUnitTest,
       AddAndApprove1559TransactionNoPermission) {
  // This makes sure the state manager gets the chain ID
  browser_task_environment_.RunUntilIdle();

  bool callback_called = false;
  provider()->AddAndApprove1559Transaction(
      mojom::TxData1559::New(
          mojom::TxData::New("0x00", "", "0x00",
                             "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c",
                             "0x00", std::vector<uint8_t>()),
          "0x04", "0x0", "0x0", nullptr),
      "0xbe862ad9abfe6f22bcb087716c7d89a26051f74d",
      base::BindLambdaForTesting([&](bool success, const std::string& hash,
                                     const std::string& error_message) {
        EXPECT_FALSE(success);
        EXPECT_TRUE(hash.empty());
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
  CreateWallet();
  AddAccount();
  GURL url("https://brave.com");
  Navigate(url);
  AddEthereumPermission(url);
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
  CreateWallet();
  AddAccount();
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
  base::RunLoop run_loop;
  provider()->RequestEthereumPermissions(base::BindLambdaForTesting(
      [&](bool success, const std::vector<std::string>& allowed_accounts) {
        EXPECT_FALSE(success);
        EXPECT_TRUE(allowed_accounts.empty());
        run_loop.Quit();
      }));
  run_loop.Run();
  EXPECT_TRUE(new_setup_callback_called);
}

TEST_F(BraveWalletProviderImplUnitTest, SignMessage) {
  CreateWallet();
  AddAccount();
  std::string signature;
  int error;
  std::string error_message;
  SignMessage(absl::nullopt, "1234", "0x1234", &signature, &error,
              &error_message);
  EXPECT_TRUE(signature.empty());
  EXPECT_EQ(error, static_cast<int>(ProviderErrors::kInvalidParams));
  EXPECT_EQ(error_message,
            l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));

  SignMessage(absl::nullopt, "0x12345678", "0x1234", &signature, &error,
              &error_message);
  EXPECT_TRUE(signature.empty());
  EXPECT_EQ(error, static_cast<int>(ProviderErrors::kInvalidParams));
  EXPECT_EQ(error_message,
            l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));

  const std::string address = "0x1234567890123456789012345678901234567890";
  SignMessage(absl::nullopt, address, "0x1234", &signature, &error,
              &error_message);
  EXPECT_TRUE(signature.empty());
  EXPECT_EQ(error, static_cast<int>(ProviderErrors::kUnauthorized));
  EXPECT_EQ(error_message,
            l10n_util::GetStringFUTF8(IDS_WALLET_ETH_SIGN_NOT_AUTHED,
                                      base::ASCIIToUTF16(address)));

  // No permission
  const std::vector<std::string> addresses = GetAddresses();
  ASSERT_FALSE(address.empty());
  SignMessage(absl::nullopt, addresses[0], "0x1234", &signature, &error,
              &error_message);
  EXPECT_TRUE(signature.empty());
  EXPECT_EQ(error, static_cast<int>(ProviderErrors::kUnauthorized));
  EXPECT_EQ(error_message,
            l10n_util::GetStringFUTF8(IDS_WALLET_ETH_SIGN_NOT_AUTHED,
                                      base::ASCIIToUTF16(addresses[0])));
  GURL url("https://brave.com");
  Navigate(url);
  AddEthereumPermission(url);
  SignMessage(true, addresses[0], "0x1234", &signature, &error, &error_message);

  EXPECT_FALSE(signature.empty());
  EXPECT_EQ(error, 0);
  EXPECT_TRUE(error_message.empty());

  // User reject request
  SignMessage(false, addresses[0], "0x1234", &signature, &error,
              &error_message);
  EXPECT_TRUE(signature.empty());
  EXPECT_EQ(error, static_cast<int>(ProviderErrors::kUserRejectedRequest));
  EXPECT_EQ(error_message,
            l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST));

  keyring_controller()->Lock();

  SignMessage(true, addresses[0], "0x1234", &signature, &error, &error_message);
  EXPECT_TRUE(signature.empty());
  EXPECT_EQ(error, static_cast<int>(ProviderErrors::kInternalError));
  EXPECT_EQ(error_message, l10n_util::GetStringUTF8(
                               IDS_BRAVE_WALLET_SIGN_MESSAGE_UNLOCK_FIRST));
}

TEST_F(BraveWalletProviderImplUnitTest, SignMessageRequestQueue) {
  CreateWallet();
  AddAccount();
  GURL url("https://brave.com");
  Navigate(url);
  AddEthereumPermission(url);
  const std::vector<std::string> addresses = GetAddresses();

  const std::string message1 = "0xbeef01";
  const std::string message2 = "0xbeef02";
  int id1 = SignMessageRequest(addresses[0], message1);
  int id2 = SignMessageRequest(addresses[0], message2);

  std::vector<uint8_t> message_bytes1;
  std::vector<uint8_t> message_bytes2;
  ASSERT_TRUE(base::HexStringToBytes(message1.substr(2), &message_bytes1));
  ASSERT_TRUE(base::HexStringToBytes(message2.substr(2), &message_bytes2));
  const std::string message1_in_queue(message_bytes1.begin(),
                                      message_bytes1.end());
  const std::string message2_in_queue(message_bytes2.begin(),
                                      message_bytes2.end());

  EXPECT_EQ(GetSignMessageQueueSize(), 2u);
  EXPECT_EQ(GetSignMessageQueueFront().id, id1);
  EXPECT_EQ(GetSignMessageQueueFront().message, message1_in_queue);

  // wrong order
  brave_wallet_service_->NotifySignMessageRequestProcessed(true, id2);
  EXPECT_EQ(GetSignMessageQueueSize(), 2u);
  EXPECT_EQ(GetSignMessageQueueFront().id, id1);
  EXPECT_EQ(GetSignMessageQueueFront().message, message1_in_queue);

  brave_wallet_service_->NotifySignMessageRequestProcessed(true, id1);
  EXPECT_EQ(GetSignMessageQueueSize(), 1u);
  EXPECT_EQ(GetSignMessageQueueFront().id, id2);
  EXPECT_EQ(GetSignMessageQueueFront().message, message2_in_queue);

  // old id
  brave_wallet_service_->NotifySignMessageRequestProcessed(true, id1);
  EXPECT_EQ(GetSignMessageQueueSize(), 1u);
  EXPECT_EQ(GetSignMessageQueueFront().id, id2);
  EXPECT_EQ(GetSignMessageQueueFront().message, message2_in_queue);

  brave_wallet_service_->NotifySignMessageRequestProcessed(true, id2);
  EXPECT_EQ(GetSignMessageQueueSize(), 0u);
}

TEST_F(BraveWalletProviderImplUnitTest, ChainChangedEvent) {
  EXPECT_FALSE(observer_->ChainChangedFired());
  SetNetwork(mojom::kRinkebyChainId);
  EXPECT_TRUE(observer_->ChainChangedFired());
  EXPECT_EQ(mojom::kRinkebyChainId, observer_->GetChainId());

  // Works a second time
  observer_->Reset();
  SetNetwork(mojom::kMainnetChainId);
  EXPECT_TRUE(observer_->ChainChangedFired());
  EXPECT_EQ(mojom::kMainnetChainId, observer_->GetChainId());
}

TEST_F(BraveWalletProviderImplUnitTest, AccountsChangedEvent) {
  CreateWallet();
  AddAccount();
  GURL url("https://brave.com");
  Navigate(url);
  EXPECT_FALSE(observer_->AccountsChangedFired());
  AddEthereumPermission(url);
  EXPECT_TRUE(observer_->AccountsChangedFired());
  EXPECT_EQ(std::vector<std::string>{from()}, observer_->GetAccounts());
  observer_->Reset();

  // Resetting the permission changes the accounts again
  ResetEthereumPermission(url);
  EXPECT_TRUE(observer_->AccountsChangedFired());
  EXPECT_EQ(std::vector<std::string>(), observer_->GetAccounts());
  observer_->Reset();

  // Multiple accounts can be returned
  AddEthereumPermission(url, 0);
  AddEthereumPermission(url, 1);
  EXPECT_TRUE(observer_->AccountsChangedFired());
  EXPECT_EQ((std::vector<std::string>{from(0), from(1)}),
            observer_->GetAccounts());
  observer_->Reset();

  // Changing the selected account re-orders things
  SetSelectedAccount(from(1));
  EXPECT_TRUE(observer_->AccountsChangedFired());
  EXPECT_EQ((std::vector<std::string>{from(1), from(0)}),
            observer_->GetAccounts());
  observer_->Reset();

  // Resetting with multiple accounts works
  ResetEthereumPermission(url, 1);
  EXPECT_TRUE(observer_->AccountsChangedFired());
  EXPECT_EQ((std::vector<std::string>{from(0)}), observer_->GetAccounts());
  observer_->Reset();

  // Does not fire for a different origin that has no permissions
  Navigate(GURL("https://bravesoftware.com"));
  AddEthereumPermission(url, 1);
  SetSelectedAccount(from(0));
  EXPECT_FALSE(observer_->AccountsChangedFired());
}

}  // namespace brave_wallet
