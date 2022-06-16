/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/solana_provider_impl.h"

#include "base/feature_list.h"
#include "base/json/json_reader.h"
#include "base/memory/raw_ptr.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/brave_wallet/brave_wallet_provider_delegate_impl.h"
#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
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
#include "chrome/test/base/testing_profile.h"
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

class TestEventsListener : public mojom::SolanaEventsListener {
 public:
  TestEventsListener() {}

  void AccountChangedEvent(
      const absl::optional<std::string>& account) override {
    if (account.has_value())
      account_ = *account;
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
  }

  void SetUp() override {
    web_contents_ =
        content::TestWebContents::Create(browser_context(), nullptr);
    permissions::PermissionRequestManager::CreateForWebContents(web_contents());
    keyring_service_ =
        KeyringServiceFactory::GetServiceForContext(browser_context());
    brave_wallet_service_ =
        brave_wallet::BraveWalletServiceFactory::GetServiceForContext(
            browser_context());
    tx_service_ =
        brave_wallet::TxServiceFactory::GetServiceForContext(browser_context());
    provider_ = std::make_unique<SolanaProviderImpl>(
        keyring_service_, brave_wallet_service_, tx_service_,
        std::make_unique<brave_wallet::BraveWalletProviderDelegateImpl>(
            web_contents(), web_contents()->GetMainFrame()));
    observer_.reset(new TestEventsListener());
    provider_->Init(observer_->GetReceiver());
  }

  content::BrowserContext* browser_context() { return &profile_; }
  content::TestWebContents* web_contents() { return web_contents_.get(); }

  void Navigate(const GURL& url) { web_contents()->NavigateAndCommit(url); }
  url::Origin GetOrigin() {
    return web_contents()->GetMainFrame()->GetLastCommittedOrigin();
  }

  std::vector<mojom::SignMessageRequestPtr> GetPendingSignMessageRequests()
      const {
    base::RunLoop run_loop;
    std::vector<mojom::SignMessageRequestPtr> requests_out;
    brave_wallet_service_->GetPendingSignMessageRequests(
        base::BindLambdaForTesting(
            [&](std::vector<mojom::SignMessageRequestPtr> requests) {
              for (const auto& request : requests)
                requests_out.push_back(request.Clone());
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
    CHECK(!keyring_service_->IsLocked());
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

  std::string Connect(absl::optional<base::Value> arg,
                      mojom::SolanaProviderError* error_out,
                      std::string* error_message_out) {
    std::string account;
    base::RunLoop run_loop;
    provider_->Connect(
        std::move(arg),
        base::BindLambdaForTesting([&](mojom::SolanaProviderError error,
                                       const std::string& error_message,
                                       const std::string& public_key) {
          if (error_out)
            *error_out = error;
          if (error_message_out)
            *error_message_out = error_message;
          account = public_key;
          run_loop.Quit();
        }));
    run_loop.Run();
    return account;
  }

  std::string SignMessage(const std::vector<uint8_t>& blob_msg,
                          const absl::optional<std::string>& display_encoding,
                          mojom::SolanaProviderError* error_out,
                          std::string* error_message_out) {
    std::string signature_out;
    base::RunLoop run_loop;
    provider_->SignMessage(
        blob_msg, display_encoding,
        base::BindLambdaForTesting([&](mojom::SolanaProviderError error,
                                       const std::string& error_message,
                                       base::Value result) {
          if (error_out)
            *error_out = error;
          if (error_message_out)
            *error_message_out = error_message;
          const std::string* signature =
              result.GetDict().FindString("signature");
          if (signature)
            signature_out = *signature;
          run_loop.Quit();
        }));
    run_loop.Run();
    return signature_out;
  }

  base::Value SignAndSendTransaction(
      const std::string& encoded_serialized_message,
      mojom::SolanaProviderError expected_error,
      const std::string& expected_error_message) {
    base::Value result_out(base::Value::Type::DICTIONARY);
    base::RunLoop run_loop;
    provider_->SignAndSendTransaction(
        encoded_serialized_message, absl::nullopt,
        base::BindLambdaForTesting([&](mojom::SolanaProviderError error,
                                       const std::string& error_message,
                                       base::Value result) {
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
      const std::string& expected_error_message) {
    std::vector<uint8_t> result_out;
    base::RunLoop run_loop;
    provider_->SignTransaction(
        encoded_serialized_message,
        base::BindLambdaForTesting([&](mojom::SolanaProviderError error,
                                       const std::string& error_message,
                                       const std::vector<uint8_t>& result) {
          EXPECT_EQ(error, expected_error);
          EXPECT_EQ(error_message, expected_error_message);
          result_out = std::move(result);
          run_loop.Quit();
        }));
    run_loop.Run();
    return result_out;
  }

  std::vector<std::vector<uint8_t>> SignAllTransactions(
      const std::vector<std::string>& encoded_serialized_messages,
      mojom::SolanaProviderError expected_error,
      const std::string& expected_error_message) {
    std::vector<std::vector<uint8_t>> result_out;
    base::RunLoop run_loop;
    provider_->SignAllTransactions(
        encoded_serialized_messages,
        base::BindLambdaForTesting(
            [&](mojom::SolanaProviderError error,
                const std::string& error_message,
                const std::vector<std::vector<uint8_t>>& result) {
              EXPECT_EQ(error, expected_error);
              EXPECT_EQ(error_message, expected_error_message);
              result_out = std::move(result);
              run_loop.Quit();
            }));
    run_loop.Run();
    return result_out;
  }

  base::Value Request(const std::string& json,
                      mojom::SolanaProviderError expected_error,
                      const std::string& expected_error_message) {
    base::Value result_out(base::Value::Type::DICTIONARY);
    auto value = base::JSONReader::Read(json);
    if (!value)
      return result_out;
    base::RunLoop run_loop;
    provider_->Request(
        value->Clone(),
        base::BindLambdaForTesting([&](mojom::SolanaProviderError error,
                                       const std::string& error_message,
                                       base::Value result) {
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

 private:
  raw_ptr<KeyringService> keyring_service_ = nullptr;
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

  Navigate(GURL("https://brave.com"));
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
}

TEST_F(SolanaProviderImplUnitTest, EagerlyConnect) {
  CreateWallet();
  AddAccount();
  std::string address = GetAddressByIndex(0);
  SetSelectedAccount(address, mojom::CoinType::SOL);

  Navigate(GURL("https://brave.com"));
  mojom::SolanaProviderError error;
  std::string error_message;
  base::Value dict(base::Value::Type::DICT);
  dict.GetDict().Set("onlyIfTrusted", true);
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
  dict.GetDict().Set("ExtraP", "aramters");
  account = Connect(dict.Clone(), &error, &error_message);
  EXPECT_EQ(account, address);
  EXPECT_EQ(error, mojom::SolanaProviderError::kSuccess);
  EXPECT_TRUE(error_message.empty());
  EXPECT_TRUE(IsConnected());
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

  base::Value dict(base::Value::Type::DICT);
  dict.GetDict().Set("onlyIfTrusted", true);
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

TEST_F(SolanaProviderImplUnitTest, GetDeserializedMessage) {
  CreateWallet();
  AddAccount();
  const std::string address = GetAddressByIndex(0);
  EXPECT_FALSE(provider_->GetDeserializedMessage("", address));

  SolanaInstruction instruction(kSolanaSystemProgramId,
                                {SolanaAccountMeta(address, true, true),
                                 SolanaAccountMeta(address, false, true)},
                                {2, 0, 0, 0, 128, 150, 152, 0, 0, 0, 0, 0});
  SolanaMessage msg("9sHcv6xwn9YkB8nxTUGKDwPwNnmqVp5oAXxU8Fdkm4J6", 0, address,
                    {instruction});

  auto serialized_msg = msg.Serialize(nullptr);
  ASSERT_TRUE(serialized_msg);

  auto deserialized_msg =
      provider_->GetDeserializedMessage(Base58Encode(*serialized_msg), address);
  EXPECT_TRUE(deserialized_msg);

  // Test current selected account is not the same as the fee payer in the
  // serialized message.
  deserialized_msg = provider_->GetDeserializedMessage(
      Base58Encode(*serialized_msg),
      "3Lu176FQzbQJCc8iL9PnmALbpMPhZeknoturApnXRDJw");
  EXPECT_FALSE(deserialized_msg);

  // Mutiple signers should return absl::nullopt.
  SolanaInstruction instruction2(
      kSolanaSystemProgramId,
      {SolanaAccountMeta(address, true, true),
       SolanaAccountMeta("3Lu176FQzbQJCc8iL9PnmALbpMPhZeknoturApnXRDJw", true,
                         true)},
      {2, 0, 0, 0, 128, 150, 152, 0, 0, 0, 0, 0});
  SolanaMessage msg2("9sHcv6xwn9YkB8nxTUGKDwPwNnmqVp5oAXxU8Fdkm4J6", 0, address,
                     {instruction2});
  serialized_msg = msg2.Serialize(nullptr);
  ASSERT_TRUE(serialized_msg);

  deserialized_msg =
      provider_->GetDeserializedMessage(Base58Encode(*serialized_msg), address);
  EXPECT_FALSE(deserialized_msg);
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
  EXPECT_EQ(value, base::Value(base::Value::Type::DICTIONARY));
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
  EXPECT_EQ(value, base::Value(base::Value::Type::DICTIONARY));
  signed_tx =
      SignTransaction("", mojom::SolanaProviderError::kInternalError,
                      l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
  EXPECT_EQ(signed_tx, std::vector<uint8_t>());
  signed_txs =
      SignAllTransactions({""}, mojom::SolanaProviderError::kInternalError,
                          l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
  EXPECT_EQ(signed_txs, std::vector<std::vector<uint8_t>>());
}

TEST_F(SolanaProviderImplUnitTest, Request) {
  // no method
  base::Value result =
      Request(R"({params: {}})", mojom::SolanaProviderError::kParsingError,
              l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));
  EXPECT_TRUE(result.GetDict().empty());

  // params not dictionary
  result = Request(R"({method: "connect", params: []})",
                   mojom::SolanaProviderError::kParsingError,
                   l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));
  EXPECT_TRUE(result.GetDict().empty());

  // no params for non connect and disconnect
  for (const std::string& method : {"signTransaction", "signAndSendTransaction",
                                    "signAllTransactions", "signMessage"}) {
    result = Request(
        base::StringPrintf(R"({method: "%s", params: {}})", method.c_str()),
        mojom::SolanaProviderError::kParsingError,
        l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));
    EXPECT_TRUE(result.GetDict().empty());
  }

  // method not found
  result =
      Request(R"({method: "newMethod", params: {}})",
              mojom::SolanaProviderError::kMethodNotFound,
              l10n_util::GetStringUTF8(IDS_WALLET_REQUEST_PROCESSING_ERROR));
  EXPECT_TRUE(result.GetDict().empty());
  result = Request(
      R"({method: "newMethod"})", mojom::SolanaProviderError::kMethodNotFound,
      l10n_util::GetStringUTF8(IDS_WALLET_REQUEST_PROCESSING_ERROR));
  EXPECT_TRUE(result.GetDict().empty());

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
    EXPECT_TRUE(result.GetDict().empty());
  }
}

}  // namespace brave_wallet
