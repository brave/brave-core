/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>

#include "brave/components/brave_wallet/browser/solana_provider_impl.h"

#include "base/feature_list.h"
#include "base/json/json_reader.h"
#include "base/memory/raw_ptr.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/brave_wallet/brave_wallet_provider_delegate_impl.h"
#include "brave/browser/brave_wallet/brave_wallet_provider_delegate_impl_helper.h"
#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/browser/brave_wallet/brave_wallet_tab_helper.h"
#include "brave/browser/brave_wallet/keyring_service_factory.h"
#include "brave/browser/brave_wallet/tx_service_factory.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/solana_account_meta.h"
#include "brave/components/brave_wallet/browser/solana_instruction.h"
#include "brave/components/brave_wallet/browser/solana_message.h"
#include "brave/components/brave_wallet/browser/tx_service.h"
#include "brave/components/brave_wallet/common/brave_wallet_constants.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/brave_wallet/common/solana_utils.h"
#include "brave/components/permissions/brave_permission_manager.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/permissions/permission_manager_factory.h"
#include "chrome/test/base/scoped_testing_local_state.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/grit/brave_components_strings.h"
#include "components/permissions/permission_request_manager.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_web_contents_factory.h"
#include "content/test/test_web_contents.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"

namespace brave_wallet {

namespace {

constexpr char kEncodedSerializedMsg[] =
    "QwE1eawcSfggJRAUzH1a5gqbULPVraB9W4m138wSFvQNmnhL4utKzctTrLQUxLVQs7RHwJhskf"
    "X6xTwbQXWhz2wavFwaZekuiAcJNNYeE36SK5JWq8SX3M6vqEAC3GW456M38RzhsQK5oVYYW69J"
    "UxtUCXVBexiK";
constexpr char kHardwareAccountAddr[] =
    "3Lu176FQzbQJCc8iL9PnmALbpMPhZeknoturApnXRDJw";

class TestEventsListener : public mojom::SolanaEventsListener {
 public:
  TestEventsListener() = default;

  void AccountChangedEvent(
      const absl::optional<std::string>& account) override {
    if (account.has_value()) {
      account_ = *account;
    }
    account_changed_fired_ = true;
  }

  bool AccountChangedFired() const {
    base::RunLoop().RunUntilIdle();
    return account_changed_fired_;
  }

  std::string GetAccount() const {
    base::RunLoop().RunUntilIdle();
    return account_;
  }

  mojo::PendingRemote<mojom::SolanaEventsListener> GetReceiver() {
    return observer_receiver_.BindNewPipeAndPassRemote();
  }

  void Reset() {
    account_.clear();
    account_changed_fired_ = false;
    EXPECT_FALSE(AccountChangedFired());
  }

  bool account_changed_fired_ = false;
  std::string account_;

 private:
  mojo::Receiver<mojom::SolanaEventsListener> observer_receiver_{this};
};

}  // namespace

class SolanaProviderImplUnitTest : public testing::Test {
 public:
  SolanaProviderImplUnitTest() {
    feature_list_.InitAndEnableFeature(
        brave_wallet::features::kBraveWalletSolanaFeature);
  }
  ~SolanaProviderImplUnitTest() override = default;

  void TearDown() override {
    provider_.reset();
    web_contents_.reset();
    profile_.SetPermissionControllerDelegate(nullptr);
  }

  void SetUp() override {
    local_state_ = std::make_unique<ScopedTestingLocalState>(
        TestingBrowserProcess::GetGlobal());
    web_contents_ =
        content::TestWebContents::Create(browser_context(), nullptr);
    brave_wallet::BraveWalletTabHelper::CreateForWebContents(
        web_contents_.get());
    brave_wallet_tab_helper()->SetSkipDelegateForTesting(true);
    permissions::PermissionRequestManager::CreateForWebContents(web_contents());
    keyring_service_ =
        KeyringServiceFactory::GetServiceForContext(browser_context());
    brave_wallet_service_ =
        brave_wallet::BraveWalletServiceFactory::GetServiceForContext(
            browser_context());
    tx_service_ =
        brave_wallet::TxServiceFactory::GetServiceForContext(browser_context());
    profile_.SetPermissionControllerDelegate(
        base::WrapUnique(static_cast<permissions::BravePermissionManager*>(
            PermissionManagerFactory::GetInstance()->BuildServiceInstanceFor(
                browser_context()))));
    provider_ = std::make_unique<SolanaProviderImpl>(
        keyring_service_, brave_wallet_service_, tx_service_,
        std::make_unique<brave_wallet::BraveWalletProviderDelegateImpl>(
            web_contents(), web_contents()->GetPrimaryMainFrame()));
    observer_ = std::make_unique<TestEventsListener>();
    provider_->Init(observer_->GetReceiver());
  }

  content::BrowserContext* browser_context() { return &profile_; }
  content::TestWebContents* web_contents() { return web_contents_.get(); }

  brave_wallet::BraveWalletTabHelper* brave_wallet_tab_helper() {
    return brave_wallet::BraveWalletTabHelper::FromWebContents(
        web_contents_.get());
  }

  void Navigate(const GURL& url) { web_contents()->NavigateAndCommit(url); }
  url::Origin GetOrigin() {
    return web_contents()->GetPrimaryMainFrame()->GetLastCommittedOrigin();
  }

  std::vector<mojom::SignMessageRequestPtr> GetPendingSignMessageRequests()
      const {
    base::RunLoop run_loop;
    std::vector<mojom::SignMessageRequestPtr> requests_out;
    brave_wallet_service_->GetPendingSignMessageRequests(
        base::BindLambdaForTesting(
            [&](std::vector<mojom::SignMessageRequestPtr> requests) {
              for (const auto& request : requests) {
                requests_out.push_back(request.Clone());
              }
              run_loop.Quit();
            }));
    run_loop.Run();
    return requests_out;
  }

  void CreateWallet() {
    base::RunLoop run_loop;
    keyring_service_->CreateWallet(
        "brave",
        base::BindLambdaForTesting([&run_loop](const std::string& mnemonic) {
          EXPECT_FALSE(mnemonic.empty());
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  void AddAccount() {
    base::RunLoop run_loop;
    keyring_service_->AddAccount(
        "New Account", mojom::CoinType::SOL,
        base::BindLambdaForTesting([&run_loop](bool success) {
          EXPECT_TRUE(success);
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  void AddHardwareAccount(const std::string& address) {
    std::vector<mojom::HardwareWalletAccountPtr> hw_accounts;
    hw_accounts.push_back(mojom::HardwareWalletAccount::New(
        address, "m/44'/501'/0'/0", "name 1", "Ledger", "device1",
        mojom::CoinType::SOL, absl::nullopt));

    keyring_service_->AddHardwareAccounts(std::move(hw_accounts));
  }

  void SetSelectedAccount(const std::string& address, mojom::CoinType coin) {
    base::RunLoop run_loop;
    keyring_service_->SetSelectedAccount(
        address, coin, base::BindLambdaForTesting([&run_loop](bool success) {
          EXPECT_TRUE(success);
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  std::string GetAddressByIndex(
      size_t index,
      const std::string& id = mojom::kSolanaKeyringId) {
    CHECK(!keyring_service_->IsLockedSync());
    return keyring_service_->GetHDKeyringById(id)->GetAddress(index);
  }

  void LockWallet() {
    keyring_service_->Lock();
    // Needed so KeyringServiceObserver::Locked handler can be hit
    // which the provider object listens to for the accountsChanged event.
    base::RunLoop().RunUntilIdle();
  }

  void UnlockWallet() {
    base::RunLoop run_loop;
    keyring_service_->Unlock(
        "brave", base::BindLambdaForTesting([&run_loop](bool success) {
          ASSERT_TRUE(success);
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  bool RemoveHardwareAccount(const std::string& address) {
    bool success;
    base::RunLoop run_loop;
    keyring_service_->RemoveHardwareAccount(
        address, mojom::CoinType::SOL, base::BindLambdaForTesting([&](bool v) {
          success = v;
          run_loop.Quit();
        }));
    run_loop.Run();
    return success;
  }

  void AddSolanaPermission(const url::Origin& origin,
                           const std::string& address) {
    base::RunLoop run_loop;
    brave_wallet_service_->AddPermission(
        mojom::CoinType::SOL, origin, address,
        base::BindLambdaForTesting([&](bool success) {
          EXPECT_TRUE(success);
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  std::string Connect(absl::optional<base::Value::Dict> arg,
                      mojom::SolanaProviderError* error_out,
                      std::string* error_message_out) {
    std::string account;
    base::RunLoop run_loop;
    provider_->Connect(
        std::move(arg),
        base::BindLambdaForTesting([&](mojom::SolanaProviderError error,
                                       const std::string& error_message,
                                       const std::string& public_key) {
          if (error_out) {
            *error_out = error;
          }
          if (error_message_out) {
            *error_message_out = error_message;
          }
          account = public_key;
          run_loop.Quit();
        }));
    run_loop.Run();
    return account;
  }

  std::string SignMessage(
      const std::vector<uint8_t>& blob_msg,
      const absl::optional<std::string>& display_encoding,
      mojom::SolanaProviderError* error_out,
      std::string* error_message_out,
      bool run_notify = false,
      bool approve = true,
      mojom::ByteArrayStringUnionPtr hw_sig = nullptr,
      const absl::optional<std::string>& err_in = absl::nullopt) {
    std::string signature_out;
    base::RunLoop run_loop;
    provider_->SignMessage(
        blob_msg, display_encoding,
        base::BindLambdaForTesting([&](mojom::SolanaProviderError error,
                                       const std::string& error_message,
                                       base::Value::Dict result) {
          if (error_out) {
            *error_out = error;
          }
          if (error_message_out) {
            *error_message_out = error_message;
          }
          const std::string* signature = result.FindString("signature");
          if (signature) {
            signature_out = *signature;
          }
          run_loop.Quit();
        }));

    if (run_notify) {
      brave_wallet_service_->NotifySignMessageRequestProcessed(
          approve, brave_wallet_service_->sign_message_id_ - 1,
          std::move(hw_sig), err_in);
    }

    run_loop.Run();
    return signature_out;
  }

  base::Value::Dict SignAndSendTransaction(
      const std::string& encoded_serialized_message,
      mojom::SolanaProviderError expected_error,
      const std::string& expected_error_message) {
    base::Value::Dict result_out;
    base::RunLoop run_loop;
    provider_->SignAndSendTransaction(
        mojom::SolanaSignTransactionParam::New(
            encoded_serialized_message,
            std::vector<mojom::SignaturePubkeyPairPtr>()),
        absl::nullopt,
        base::BindLambdaForTesting([&](mojom::SolanaProviderError error,
                                       const std::string& error_message,
                                       base::Value::Dict result) {
          EXPECT_EQ(error, expected_error);
          EXPECT_EQ(error_message, expected_error_message);
          result_out = std::move(result);
          run_loop.Quit();
        }));
    run_loop.Run();
    return result_out;
  }

  std::vector<uint8_t> SignTransaction(
      const std::string& encoded_serialized_message,
      mojom::SolanaProviderError expected_error,
      const std::string& expected_error_message,
      bool run_notify = false,
      bool approve = true,
      mojom::ByteArrayStringUnionPtr hw_sig = nullptr,
      const absl::optional<std::string>& err_in = absl::nullopt) {
    std::vector<uint8_t> result_out;
    base::RunLoop run_loop;
    provider_->SignTransaction(
        mojom::SolanaSignTransactionParam::New(
            encoded_serialized_message,
            std::vector<mojom::SignaturePubkeyPairPtr>()),
        base::BindLambdaForTesting([&](mojom::SolanaProviderError error,
                                       const std::string& error_message,
                                       const std::vector<uint8_t>& result,
                                       mojom::SolanaMessageVersion version) {
          EXPECT_EQ(error, expected_error);
          EXPECT_EQ(error_message, expected_error_message);
          result_out = std::move(result);
          run_loop.Quit();
        }));

    if (run_notify) {
      brave_wallet_service_->NotifySignTransactionRequestProcessed(
          approve, brave_wallet_service_->sign_transaction_id_ - 1,
          std::move(hw_sig), err_in);
    }

    run_loop.Run();
    return result_out;
  }

  std::vector<std::vector<uint8_t>> SignAllTransactions(
      const std::vector<std::string>& encoded_serialized_messages,
      mojom::SolanaProviderError expected_error,
      const std::string& expected_error_message,
      bool run_notify = false,
      bool approve = true,
      absl::optional<std::vector<mojom::ByteArrayStringUnionPtr>> hw_sigs =
          absl::nullopt,
      const absl::optional<std::string>& err_in = absl::nullopt) {
    std::vector<std::vector<uint8_t>> result_out;
    base::RunLoop run_loop;
    std::vector<mojom::SolanaSignTransactionParamPtr> params;
    for (const auto& encoded_serialized_message : encoded_serialized_messages) {
      params.push_back(mojom::SolanaSignTransactionParam::New(
          encoded_serialized_message,
          std::vector<mojom::SignaturePubkeyPairPtr>()));
    }
    provider_->SignAllTransactions(
        std::move(params),
        base::BindLambdaForTesting(
            [&](mojom::SolanaProviderError error,
                const std::string& error_message,
                const std::vector<std::vector<uint8_t>>& result,
                const std::vector<mojom::SolanaMessageVersion>& versions) {
              EXPECT_EQ(error, expected_error);
              EXPECT_EQ(error_message, expected_error_message);
              result_out = std::move(result);
              run_loop.Quit();
            }));

    if (run_notify) {
      brave_wallet_service_->NotifySignAllTransactionsRequestProcessed(
          approve, brave_wallet_service_->sign_all_transactions_id_ - 1,
          std::move(hw_sigs), err_in);
    }

    run_loop.Run();
    return result_out;
  }

  base::Value::Dict Request(const std::string& json,
                            mojom::SolanaProviderError expected_error,
                            const std::string& expected_error_message) {
    base::Value::Dict result_out;
    auto value = base::JSONReader::Read(json);
    if (!value) {
      return result_out;
    }
    base::RunLoop run_loop;
    provider_->Request(
        value->GetDict().Clone(),
        base::BindLambdaForTesting([&](mojom::SolanaProviderError error,
                                       const std::string& error_message,
                                       base::Value::Dict result) {
          EXPECT_EQ(error, expected_error);
          EXPECT_EQ(error_message, expected_error_message);
          result_out = std::move(result);
          run_loop.Quit();
        }));
    run_loop.Run();
    return result_out;
  }

  bool IsConnected() {
    bool result = false;
    base::RunLoop run_loop;
    provider_->IsConnected(base::BindLambdaForTesting([&](bool is_connected) {
      result = is_connected;
      run_loop.Quit();
    }));
    run_loop.Run();
    return result;
  }

 protected:
  std::unique_ptr<SolanaProviderImpl> provider_;
  std::unique_ptr<TestEventsListener> observer_;
  raw_ptr<KeyringService> keyring_service_ = nullptr;

 private:
  std::unique_ptr<ScopedTestingLocalState> local_state_;
  raw_ptr<BraveWalletService> brave_wallet_service_ = nullptr;
  raw_ptr<TxService> tx_service_ = nullptr;
  std::unique_ptr<content::TestWebContents> web_contents_;
  content::BrowserTaskEnvironment browser_task_environment_;
  content::TestWebContentsFactory factory_;
  TestingProfile profile_;
  base::test::ScopedFeatureList feature_list_;
};

TEST_F(SolanaProviderImplUnitTest, Connect) {
  CreateWallet();
  AddAccount();
  std::string address = GetAddressByIndex(0);
  SetSelectedAccount(address, mojom::CoinType::SOL);

  mojom::SolanaProviderError error;
  std::string error_message;
  // no permission, trigger
  // permissions::BraveWalletPermissionContext::RequestPermissions failed
  std::string account = Connect(absl::nullopt, &error, &error_message);
  EXPECT_TRUE(account.empty());
  EXPECT_EQ(error, mojom::SolanaProviderError::kInternalError);
  EXPECT_FALSE(IsConnected());

  GURL url("https://brave.com");
  Navigate(url);
  AddSolanaPermission(GetOrigin(), address);
  account = Connect(absl::nullopt, &error, &error_message);
  EXPECT_EQ(account, address);
  EXPECT_EQ(error, mojom::SolanaProviderError::kSuccess);
  EXPECT_TRUE(error_message.empty());
  EXPECT_TRUE(IsConnected());

  provider_->Disconnect();
  mojom::SolanaProviderError pending_error;
  std::string pending_error_message;
  std::string pending_connect_account;
  LockWallet();
  base::RunLoop run_loop;
  provider_->Connect(absl::nullopt, base::BindLambdaForTesting(
                                        [&pending_error, &pending_error_message,
                                         &pending_connect_account, &run_loop](
                                            mojom::SolanaProviderError error,
                                            const std::string& error_message,
                                            const std::string& public_key) {
                                          pending_error = error;
                                          pending_error_message = error_message;
                                          pending_connect_account = public_key;
                                          run_loop.Quit();
                                        }));
  // Request will be rejected because it is still waiting for wallet unlock.
  account = Connect(absl::nullopt, &error, &error_message);
  EXPECT_TRUE(account.empty());
  EXPECT_EQ(error, mojom::SolanaProviderError::kUserRejectedRequest);
  EXPECT_EQ(error_message,
            l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST));
  EXPECT_FALSE(IsConnected());
  // Unlock wallet will continue previous connect, since permission is already
  // granted we don't need to grant it again.
  UnlockWallet();

  // Wait for Unlocked observer to pick up previous connect
  run_loop.Run();
  EXPECT_EQ(pending_connect_account, address);
  EXPECT_EQ(pending_error, mojom::SolanaProviderError::kSuccess);
  EXPECT_TRUE(pending_error_message.empty());
  EXPECT_TRUE(IsConnected());

  // CONTENT_SETTING_BLOCK will rule out previous granted permission.
  scoped_refptr<HostContentSettingsMap> map =
      HostContentSettingsMapFactory::GetForProfile(browser_context());
  ASSERT_TRUE(map);
  map->SetContentSettingDefaultScope(
      url, url, ContentSettingsType::BRAVE_SOLANA, CONTENT_SETTING_BLOCK);
  account = Connect(absl::nullopt, &error, &error_message);
  EXPECT_TRUE(account.empty());
  EXPECT_EQ(error, mojom::SolanaProviderError::kUserRejectedRequest);
  EXPECT_EQ(error_message,
            l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST));
  // When CONTENT_SETTING_BLOCK is removed, previously granted permission works
  // again.
  map->SetContentSettingDefaultScope(
      url, url, ContentSettingsType::BRAVE_SOLANA, CONTENT_SETTING_DEFAULT);
  account = Connect(absl::nullopt, &error, &error_message);
  EXPECT_EQ(account, address);
  EXPECT_EQ(error, mojom::SolanaProviderError::kSuccess);
  EXPECT_TRUE(error_message.empty());
}

TEST_F(SolanaProviderImplUnitTest, EagerlyConnect) {
  CreateWallet();
  AddAccount();
  std::string address = GetAddressByIndex(0);
  SetSelectedAccount(address, mojom::CoinType::SOL);

  Navigate(GURL("https://brave.com"));
  mojom::SolanaProviderError error;
  std::string error_message;
  base::Value::Dict dict;
  dict.Set("onlyIfTrusted", true);
  // no permission will be rejected automatically
  std::string account = Connect(dict.Clone(), &error, &error_message);
  EXPECT_TRUE(account.empty());
  EXPECT_EQ(error, mojom::SolanaProviderError::kUserRejectedRequest);
  EXPECT_EQ(error_message,
            l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST));
  EXPECT_FALSE(IsConnected());
  // Request will be rejected when wallet is locked (no permission)
  LockWallet();
  account = Connect(dict.Clone(), &error, &error_message);
  EXPECT_TRUE(account.empty());
  EXPECT_EQ(error, mojom::SolanaProviderError::kUserRejectedRequest);
  EXPECT_EQ(error_message,
            l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST));
  EXPECT_FALSE(IsConnected());
  UnlockWallet();

  AddSolanaPermission(GetOrigin(), address);
  // Request will be rejected when wallet is locked (has permission)
  LockWallet();
  account = Connect(dict.Clone(), &error, &error_message);
  EXPECT_TRUE(account.empty());
  EXPECT_EQ(error, mojom::SolanaProviderError::kUserRejectedRequest);
  EXPECT_EQ(error_message,
            l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST));
  EXPECT_FALSE(IsConnected());
  UnlockWallet();

  // extra parameters doesn't matter
  dict.Set("ExtraP", "aramters");
  account = Connect(dict.Clone(), &error, &error_message);
  EXPECT_EQ(account, address);
  EXPECT_EQ(error, mojom::SolanaProviderError::kSuccess);
  EXPECT_TRUE(error_message.empty());
  EXPECT_TRUE(IsConnected());
}

TEST_F(SolanaProviderImplUnitTest, ConnectWithNoSolanaAccount) {
  bool account_creation_callback_called = false;
  SetCallbackForAccountCreationForTesting(base::BindLambdaForTesting(
      [&]() { account_creation_callback_called = true; }));
  Navigate(GURL("https://brave.com"));

  mojom::SolanaProviderError error;
  std::string error_message;
  // No wallet setup
  std::string account = Connect(absl::nullopt, &error, &error_message);
  EXPECT_TRUE(account.empty());
  EXPECT_EQ(error, mojom::SolanaProviderError::kInternalError);
  EXPECT_FALSE(IsConnected());
  EXPECT_TRUE(account_creation_callback_called);
  EXPECT_TRUE(provider_->account_creation_shown_);

  provider_->account_creation_shown_ = false;
  account_creation_callback_called = false;
  SetCallbackForAccountCreationForTesting(base::BindLambdaForTesting(
      [&]() { account_creation_callback_called = true; }));
  // No solana account
  CreateWallet();
  keyring_service_->RemoveSelectedAccountForCoin(mojom::CoinType::SOL,
                                                 mojom::kSolanaKeyringId);
  account = Connect(absl::nullopt, &error, &error_message);
  EXPECT_TRUE(account.empty());
  EXPECT_EQ(error, mojom::SolanaProviderError::kInternalError);
  EXPECT_FALSE(IsConnected());
  EXPECT_TRUE(account_creation_callback_called);
  EXPECT_TRUE(provider_->account_creation_shown_);

  // It should be shown at most once.
  account_creation_callback_called = false;
  SetCallbackForAccountCreationForTesting(base::BindLambdaForTesting(
      [&]() { account_creation_callback_called = true; }));
  account = Connect(absl::nullopt, &error, &error_message);
  EXPECT_TRUE(account.empty());
  EXPECT_EQ(error, mojom::SolanaProviderError::kInternalError);
  EXPECT_FALSE(IsConnected());
  EXPECT_FALSE(account_creation_callback_called);
  EXPECT_TRUE(provider_->account_creation_shown_);
  // Clear previous set callback which won't run in this test suite
  SetCallbackForAccountCreationForTesting(base::DoNothing());
}

TEST_F(SolanaProviderImplUnitTest, Disconnect) {
  CreateWallet();
  AddAccount();
  std::string address = GetAddressByIndex(0);
  SetSelectedAccount(address, mojom::CoinType::SOL);

  Navigate(GURL("https://brave.com"));
  AddSolanaPermission(GetOrigin(), address);
  std::string account = Connect(absl::nullopt, nullptr, nullptr);
  ASSERT_TRUE(!account.empty());
  ASSERT_TRUE(IsConnected());

  provider_->Disconnect();
  EXPECT_FALSE(IsConnected());
}

TEST_F(SolanaProviderImplUnitTest,
       AccountChangedEvent_RemoveSelectedHardwareAccount) {
  ASSERT_FALSE(observer_->AccountChangedFired());
  CreateWallet();
  AddHardwareAccount(kHardwareAccountAddr);
  EXPECT_FALSE(observer_->AccountChangedFired());

  SetSelectedAccount(kHardwareAccountAddr, mojom::CoinType::SOL);
  EXPECT_TRUE(observer_->AccountChangedFired());
  observer_->Reset();

  // Connect the account.
  Navigate(GURL("https://brave.com"));
  AddSolanaPermission(GetOrigin(), kHardwareAccountAddr);
  std::string account = Connect(absl::nullopt, nullptr, nullptr);
  ASSERT_TRUE(!account.empty());
  ASSERT_TRUE(IsConnected());

  // Remove selected hardware account.
  EXPECT_TRUE(RemoveHardwareAccount(kHardwareAccountAddr));
  EXPECT_TRUE(observer_->AccountChangedFired());
  // Account is empty because GetSelectedAccount returns absl::nullopt.
  EXPECT_TRUE(observer_->GetAccount().empty());
  observer_->Reset();
}

TEST_F(SolanaProviderImplUnitTest, AccountChangedEvent) {
  ASSERT_FALSE(observer_->AccountChangedFired());
  CreateWallet();
  AddAccount();
  EXPECT_FALSE(observer_->AccountChangedFired());

  std::string address = GetAddressByIndex(0);
  SetSelectedAccount(address, mojom::CoinType::SOL);
  EXPECT_TRUE(observer_->AccountChangedFired());
  // since it is not connected, account is empty
  EXPECT_TRUE(observer_->GetAccount().empty());
  // connect the account
  Navigate(GURL("https://brave.com"));
  AddSolanaPermission(GetOrigin(), address);
  std::string account = Connect(absl::nullopt, nullptr, nullptr);
  ASSERT_TRUE(!account.empty());
  ASSERT_TRUE(IsConnected());

  // add another account
  observer_->Reset();
  AddAccount();
  EXPECT_FALSE(observer_->AccountChangedFired());

  SetSelectedAccount(GetAddressByIndex(1), mojom::CoinType::SOL);
  EXPECT_TRUE(observer_->AccountChangedFired());
  // since it is not connected, account is empty
  EXPECT_TRUE(observer_->GetAccount().empty());

  observer_->Reset();
  // now switch back to the account just connected
  SetSelectedAccount(address, mojom::CoinType::SOL);
  EXPECT_TRUE(observer_->AccountChangedFired());
  EXPECT_EQ(observer_->GetAccount(), address);

  observer_->Reset();
  // select non SOL account
  std::string eth_address = GetAddressByIndex(0, mojom::kDefaultKeyringId);
  SetSelectedAccount(eth_address, mojom::CoinType::ETH);
  EXPECT_FALSE(observer_->AccountChangedFired());
}

TEST_F(SolanaProviderImplUnitTest, NoSelectedAccount) {
  Navigate(GURL("https://brave.com"));
  mojom::SolanaProviderError error;
  std::string error_message;
  // connect
  std::string account = Connect(absl::nullopt, &error, &error_message);
  EXPECT_TRUE(account.empty());
  EXPECT_EQ(error, mojom::SolanaProviderError::kInternalError);
  EXPECT_EQ(error_message, l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
  EXPECT_FALSE(IsConnected());

  base::Value::Dict dict;
  dict.Set("onlyIfTrusted", true);
  // eagerly connect
  account = Connect(dict.Clone(), &error, &error_message);
  EXPECT_TRUE(account.empty());
  EXPECT_EQ(error, mojom::SolanaProviderError::kInternalError);
  EXPECT_EQ(error_message, l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
  EXPECT_FALSE(IsConnected());

  // sign message
  const std::string signature =
      SignMessage({1, 2, 3, 4}, absl::nullopt, &error, &error_message);
  EXPECT_TRUE(signature.empty());
  EXPECT_EQ(error, mojom::SolanaProviderError::kInternalError);
  EXPECT_EQ(error_message, l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
}

TEST_F(SolanaProviderImplUnitTest, SignMessage) {
  CreateWallet();
  AddAccount();
  std::string address = GetAddressByIndex(0);
  SetSelectedAccount(address, mojom::CoinType::SOL);
  Navigate(GURL("https://brave.com"));

  mojom::SolanaProviderError error;
  std::string error_message;

  // Disconnected state will be rejcted.
  ASSERT_FALSE(IsConnected());
  std::string signature =
      SignMessage({1, 2, 3, 4}, absl::nullopt, &error, &error_message);
  EXPECT_TRUE(signature.empty());
  EXPECT_EQ(error, mojom::SolanaProviderError::kUnauthorized);
  EXPECT_EQ(error_message, l10n_util::GetStringUTF8(IDS_WALLET_NOT_AUTHED));

  // transaction payload message will be rejected
  signature = SignMessage(
      {1,   0,   1,   3,   161, 51,  89,  91,  115, 210, 217, 212, 76,  159,
       171, 200, 40,  150, 157, 70,  197, 71,  24,  44,  209, 108, 143, 4,
       58,  251, 215, 62,  201, 172, 159, 197, 255, 224, 228, 245, 94,  238,
       23,  132, 206, 40,  82,  249, 219, 203, 103, 158, 110, 219, 93,  249,
       143, 134, 207, 172, 179, 76,  67,  6,   169, 164, 149, 38,  0,   0,
       0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
       0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
       0,   0,   131, 191, 83,  201, 108, 193, 222, 255, 176, 67,  136, 209,
       219, 42,  6,   169, 240, 137, 142, 185, 169, 6,   17,  87,  123, 6,
       42,  55,  162, 64,  120, 91,  1,   2,   2,   0,   1,   12,  2,   0,
       0,   0,   128, 150, 152, 0,   0,   0,   0,   0},
      absl::nullopt, &error, &error_message);
  EXPECT_TRUE(signature.empty());
  EXPECT_EQ(error, mojom::SolanaProviderError::kUnauthorized);
  EXPECT_EQ(error_message, l10n_util::GetStringUTF8(IDS_WALLET_NOT_AUTHED));

  AddSolanaPermission(GetOrigin(), address);
  Connect(absl::nullopt, &error, &error_message);
  ASSERT_TRUE(IsConnected());

  // User rejected.
  signature = SignMessage({1, 2, 3, 4}, absl::nullopt, &error, &error_message,
                          true, false /* approve */);
  EXPECT_EQ(error, mojom::SolanaProviderError::kUserRejectedRequest);
  EXPECT_EQ(error_message,
            l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST));

  // test encoding, sign message requests won't be processed so callbacks will
  // not run
  const std::vector<uint8_t> message = {66, 82, 65, 86, 69};
  provider_->SignMessage(message, absl::nullopt, base::DoNothing());
  provider_->SignMessage(message, "utf8", base::DoNothing());
  provider_->SignMessage(message, "hex", base::DoNothing());
  provider_->SignMessage(message, "invalid", base::DoNothing());

  // wait for requests getting added
  base::RunLoop().RunUntilIdle();
  auto requests = GetPendingSignMessageRequests();
  ASSERT_EQ(requests.size(), 4u);
  EXPECT_EQ(requests[0]->message, "BRAVE");
  EXPECT_EQ(requests[1]->message, "BRAVE");
  EXPECT_EQ(requests[2]->message, "0x4252415645");
  EXPECT_EQ(requests[3]->message, "BRAVE");
}

TEST_F(SolanaProviderImplUnitTest, SignMessage_Hardware) {
  mojom::SolanaProviderError error;
  std::string error_message;
  auto mock_hw_sig = mojom::ByteArrayStringUnion::NewBytes(
      std::vector<uint8_t>(kSolanaSignatureSize, 1));
  const std::vector<uint8_t> mock_msg({1, 2, 3, 4});

  CreateWallet();
  AddHardwareAccount(kHardwareAccountAddr);
  SetSelectedAccount(kHardwareAccountAddr, mojom::CoinType::SOL);
  Navigate(GURL("https://brave.com"));

  AddSolanaPermission(GetOrigin(), kHardwareAccountAddr);
  Connect(absl::nullopt, &error, &error_message);
  ASSERT_TRUE(IsConnected());

  // User accepted.
  std::string signature =
      SignMessage(mock_msg, absl::nullopt, &error, &error_message, true, true,
                  mock_hw_sig.Clone());
  EXPECT_EQ(signature, Base58Encode(mock_hw_sig->get_bytes()));
  EXPECT_EQ(error, mojom::SolanaProviderError::kSuccess);
  EXPECT_TRUE(error_message.empty());

  // User rejected.
  signature =
      SignMessage(mock_msg, absl::nullopt, &error, &error_message, true, false);
  EXPECT_EQ(error, mojom::SolanaProviderError::kUserRejectedRequest);
  EXPECT_EQ(error_message,
            l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST));

  // Hareware signing has error.
  signature = SignMessage(mock_msg, absl::nullopt, &error, &error_message, true,
                          true, mock_hw_sig.Clone(), "error");
  EXPECT_EQ(error, mojom::SolanaProviderError::kInternalError);
  EXPECT_EQ(error_message, "error");

  // Invalid signatures: null signature, empty signature, non-bytes signature.
  std::vector<mojom::ByteArrayStringUnionPtr> invalid_sigs;
  invalid_sigs.push_back(nullptr);
  invalid_sigs.push_back(mojom::ByteArrayStringUnion::NewBytes({}));
  invalid_sigs.push_back(mojom::ByteArrayStringUnion::NewStr("str"));

  for (auto& invalid_sig : invalid_sigs) {
    signature = SignMessage(mock_msg, absl::nullopt, &error, &error_message,
                            true, true, std::move(invalid_sig));
    EXPECT_EQ(error, mojom::SolanaProviderError::kInternalError);
    EXPECT_EQ(error_message,
              l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
  }
}

TEST_F(SolanaProviderImplUnitTest, GetDeserializedMessage) {
  CreateWallet();
  AddAccount();
  const std::string address = GetAddressByIndex(0);
  EXPECT_FALSE(provider_->GetDeserializedMessage("", address));

  SolanaInstruction instruction(
      mojom::kSolanaSystemProgramId,
      {SolanaAccountMeta(address, absl::nullopt, true, true),
       SolanaAccountMeta(address, absl::nullopt, false, true)},
      {2, 0, 0, 0, 128, 150, 152, 0, 0, 0, 0, 0});
  auto msg = SolanaMessage::CreateLegacyMessage(
      "9sHcv6xwn9YkB8nxTUGKDwPwNnmqVp5oAXxU8Fdkm4J6", 0, address,
      {instruction});
  ASSERT_TRUE(msg);
  auto serialized_msg = msg->Serialize(nullptr);
  ASSERT_TRUE(serialized_msg);

  auto deserialized_msg =
      provider_->GetDeserializedMessage(Base58Encode(*serialized_msg), address);
  EXPECT_TRUE(deserialized_msg);

  // Test current selected account is not the same as the fee payer in the
  // serialized message.
  deserialized_msg = provider_->GetDeserializedMessage(
      Base58Encode(*serialized_msg),
      "3Lu176FQzbQJCc8iL9PnmALbpMPhZeknoturApnXRDJw");
  EXPECT_TRUE(deserialized_msg);
}

TEST_F(SolanaProviderImplUnitTest, SignTransactionAPIs) {
  CreateWallet();
  AddAccount();
  std::string address = GetAddressByIndex(0);
  SetSelectedAccount(address, mojom::CoinType::SOL);
  Navigate(GURL("https://brave.com"));

  // Disconnected state will be rejcted.
  ASSERT_FALSE(IsConnected());
  auto value = SignAndSendTransaction(
      kEncodedSerializedMsg, mojom::SolanaProviderError::kUnauthorized,
      l10n_util::GetStringUTF8(IDS_WALLET_NOT_AUTHED));
  EXPECT_EQ(value, base::Value::Dict());
  auto signed_tx = SignTransaction(
      kEncodedSerializedMsg, mojom::SolanaProviderError::kUnauthorized,
      l10n_util::GetStringUTF8(IDS_WALLET_NOT_AUTHED));
  EXPECT_EQ(signed_tx, std::vector<uint8_t>());
  auto signed_txs = SignAllTransactions(
      {kEncodedSerializedMsg}, mojom::SolanaProviderError::kUnauthorized,
      l10n_util::GetStringUTF8(IDS_WALLET_NOT_AUTHED));
  EXPECT_EQ(signed_txs, std::vector<std::vector<uint8_t>>());

  AddSolanaPermission(GetOrigin(), address);
  Connect(absl::nullopt, nullptr, nullptr);
  ASSERT_TRUE(IsConnected());

  // Test message can't be deserialized.
  value = SignAndSendTransaction(
      "", mojom::SolanaProviderError::kInternalError,
      l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
  EXPECT_EQ(value, base::Value::Dict());
  signed_tx =
      SignTransaction("", mojom::SolanaProviderError::kInternalError,
                      l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
  EXPECT_EQ(signed_tx, std::vector<uint8_t>());
  signed_txs =
      SignAllTransactions({""}, mojom::SolanaProviderError::kInternalError,
                          l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
  EXPECT_EQ(signed_txs, std::vector<std::vector<uint8_t>>());
}

TEST_F(SolanaProviderImplUnitTest, SignTransactionAPIs_Hardware) {
  CreateWallet();
  AddHardwareAccount(kHardwareAccountAddr);
  SetSelectedAccount(kHardwareAccountAddr, mojom::CoinType::SOL);
  Navigate(GURL("https://brave.com"));
  AddSolanaPermission(GetOrigin(), kHardwareAccountAddr);
  Connect(absl::nullopt, nullptr, nullptr);
  ASSERT_TRUE(IsConnected());

  SolanaInstruction instruction(
      mojom::kSolanaSystemProgramId,
      {SolanaAccountMeta(kHardwareAccountAddr, absl::nullopt, true, true),
       SolanaAccountMeta(kHardwareAccountAddr, absl::nullopt, false, true)},
      {});
  auto msg = SolanaMessage::CreateLegacyMessage(
      "9sHcv6xwn9YkB8nxTUGKDwPwNnmqVp5oAXxU8Fdkm4J6", 0, kHardwareAccountAddr,
      {instruction});
  ASSERT_TRUE(msg);
  auto serialized_msg = msg->Serialize(nullptr);
  ASSERT_TRUE(serialized_msg);
  auto encoded_serialized_msg = Base58Encode(*serialized_msg);

  auto mock_hw_sig = mojom::ByteArrayStringUnion::NewBytes(
      std::vector<uint8_t>(kSolanaSignatureSize, 2));
  std::vector<mojom::ByteArrayStringUnionPtr> mock_hw_sigs;
  mock_hw_sigs.push_back(mock_hw_sig.Clone());
  mock_hw_sigs.push_back(mock_hw_sig.Clone());
  std::vector<uint8_t> expected_signed_tx = {1};  // size of sig array
  expected_signed_tx.insert(expected_signed_tx.end(),
                            mock_hw_sig->get_bytes().begin(),
                            mock_hw_sig->get_bytes().end());
  expected_signed_tx.insert(expected_signed_tx.end(), serialized_msg->begin(),
                            serialized_msg->end());
  std::vector<std::vector<uint8_t>> expected_signed_txs;
  expected_signed_txs.push_back(expected_signed_tx);
  expected_signed_txs.push_back(expected_signed_tx);

  // User accepted.
  auto signed_tx = SignTransaction(encoded_serialized_msg,
                                   mojom::SolanaProviderError::kSuccess, "",
                                   true, true, mock_hw_sig.Clone());
  EXPECT_EQ(signed_tx, expected_signed_tx);

  auto signed_txs =
      SignAllTransactions({encoded_serialized_msg, encoded_serialized_msg},
                          mojom::SolanaProviderError::kSuccess, "", true, true,
                          std::move(mock_hw_sigs));
  EXPECT_EQ(signed_txs, expected_signed_txs);

  // User rejected.
  signed_tx = SignTransaction(
      encoded_serialized_msg, mojom::SolanaProviderError::kUserRejectedRequest,
      l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST), true, false);
  EXPECT_TRUE(signed_tx.empty());
  signed_txs = SignAllTransactions(
      {encoded_serialized_msg},
      mojom::SolanaProviderError::kUserRejectedRequest,
      l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST), true, false);
  EXPECT_TRUE(signed_txs.empty());

  // Hardware signning has error.
  signed_tx = SignTransaction(encoded_serialized_msg,
                              mojom::SolanaProviderError::kInternalError,
                              "error", true, true, nullptr, "error");
  EXPECT_TRUE(signed_tx.empty());
  signed_txs = SignAllTransactions({encoded_serialized_msg},
                                   mojom::SolanaProviderError::kInternalError,
                                   "error", true, true, absl::nullopt, "error");
  EXPECT_TRUE(signed_txs.empty());

  // Invalid signatures: null signature, empty signature, non-bytes signature.
  std::vector<mojom::ByteArrayStringUnionPtr> invalid_sigs;
  invalid_sigs.push_back(nullptr);
  invalid_sigs.push_back(mojom::ByteArrayStringUnion::NewBytes({}));
  invalid_sigs.push_back(mojom::ByteArrayStringUnion::NewStr("str"));

  for (auto& invalid_sig : invalid_sigs) {
    signed_tx = SignTransaction(
        encoded_serialized_msg, mojom::SolanaProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR), true, true,
        invalid_sig.Clone());
    std::vector<mojom::ByteArrayStringUnionPtr> sigs;
    sigs.push_back(std::move(invalid_sig));
    signed_txs = SignAllTransactions(
        {encoded_serialized_msg}, mojom::SolanaProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR), true, true,
        std::move(sigs));
  }
}

TEST_F(SolanaProviderImplUnitTest, Request) {
  // no method
  base::Value::Dict result =
      Request(R"({params: {}})", mojom::SolanaProviderError::kParsingError,
              l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));
  EXPECT_TRUE(result.empty());

  // params not dictionary
  result = Request(R"({method: "connect", params: []})",
                   mojom::SolanaProviderError::kParsingError,
                   l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));
  EXPECT_TRUE(result.empty());

  // no params for non connect and disconnect
  for (const std::string& method : {"signTransaction", "signAndSendTransaction",
                                    "signAllTransactions", "signMessage"}) {
    result = Request(
        base::StringPrintf(R"({method: "%s", params: {}})", method.c_str()),
        mojom::SolanaProviderError::kParsingError,
        l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));
    EXPECT_TRUE(result.empty());
  }

  // method not found
  result =
      Request(R"({method: "newMethod", params: {}})",
              mojom::SolanaProviderError::kMethodNotFound,
              l10n_util::GetStringUTF8(IDS_WALLET_REQUEST_PROCESSING_ERROR));
  EXPECT_TRUE(result.empty());
  result = Request(
      R"({method: "newMethod"})", mojom::SolanaProviderError::kMethodNotFound,
      l10n_util::GetStringUTF8(IDS_WALLET_REQUEST_PROCESSING_ERROR));
  EXPECT_TRUE(result.empty());

  for (const std::string& method : {"signTransaction", "signAndSendTransaction",
                                    "signAllTransactions", "signMessage"}) {
    constexpr char json[] =
        R"({method: %s,
            params: {message: %s}
        })";

    // errors should be propagated
    result =
        Request(base::StringPrintf(json, method.c_str(), kEncodedSerializedMsg),
                mojom::SolanaProviderError::kUnauthorized,
                l10n_util::GetStringUTF8(IDS_WALLET_NOT_AUTHED));
    EXPECT_TRUE(result.empty());
  }
}

}  // namespace brave_wallet
