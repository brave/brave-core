/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/brave_wallet_service.h"

#include "base/test/bind.h"
#include "brave/browser/brave_wallet/brave_wallet_importer_delegate_impl.h"
#include "brave/components/brave_wallet/browser/brave_wallet_importer_delegate.h"
#include "brave/components/brave_wallet/browser/erc_token_list_parser.h"
#include "brave/components/brave_wallet/browser/erc_token_registry.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "chrome/browser/prefs/browser_prefs.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "components/prefs/pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

const char token_list_json[] = R"(
  {
   "0x06012c8cf97BEaD5deAe237070F9587f8E7A266d": {
     "name": "Crypto Kitties",
     "logo": "CryptoKitties-Kitty-13733.svg",
     "erc20": false,
     "erc721": true,
     "symbol": "CK",
     "decimals": 0
   },
   "0x1f9840a85d5aF5bf1D1762F925BDADdC4201F984": {
     "name": "Uniswap",
     "logo": "uni.svg",
     "erc20": true,
     "symbol": "UNI",
     "decimals": 18
   }
  })";

}  // namespace

namespace brave_wallet {

class BraveWalletServiceUnitTest : public testing::Test {
 public:
  BraveWalletServiceUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}
  ~BraveWalletServiceUnitTest() override = default;

 protected:
  void SetUp() override {
    TestingProfile::Builder builder;
    auto prefs =
        std::make_unique<sync_preferences::TestingPrefServiceSyncable>();
    RegisterUserProfilePrefs(prefs->registry());
    builder.SetPrefService(std::move(prefs));
    profile_ = builder.Build();
    service_.reset(new BraveWalletService(
#if defined(OS_ANDROID)
        std::make_unique<BraveWalletImporterDelegate>(),
#else
        std::make_unique<BraveWalletImporterDelegateImpl>(profile_.get()),
#endif
        GetPrefs()));

    auto* registry = ERCTokenRegistry::GetInstance();
    std::vector<mojom::ERCTokenPtr> input_erc_tokens;
    ASSERT_TRUE(ParseTokenList(token_list_json, &input_erc_tokens));
    registry->UpdateTokenList(std::move(input_erc_tokens));

    bool callback_called = false;
    mojom::ERCTokenPtr token1;
    GetRegistry()->GetTokenByContract(
        "0x06012c8cf97BEaD5deAe237070F9587f8E7A266d",
        base::BindLambdaForTesting([&](mojom::ERCTokenPtr token) {
          token1_ = std::move(token);
          callback_called = true;
        }));
    base::RunLoop().RunUntilIdle();
    ASSERT_TRUE(callback_called);
    ASSERT_EQ(token1_->symbol, "CK");

    callback_called = false;
    mojom::ERCTokenPtr token2;
    GetRegistry()->GetTokenByContract(
        "0x1f9840a85d5aF5bf1D1762F925BDADdC4201F984",
        base::BindLambdaForTesting([&](mojom::ERCTokenPtr token) {
          token2_ = std::move(token);
          callback_called = true;
        }));
    base::RunLoop().RunUntilIdle();
    ASSERT_TRUE(callback_called);
    ASSERT_EQ(token2_->symbol, "UNI");

    eth_token_ = mojom::ERCToken::New();
    eth_token_->contract_address = "eth";
    eth_token_->name = "Ethereum";
    eth_token_->symbol = "ETH";
    eth_token_->is_erc20 = false;
    eth_token_->is_erc721 = false;
    eth_token_->decimals = 18;
    eth_token_->visible = true;

    bat_token_ = mojom::ERCToken::New();
    bat_token_->contract_address = "0x0D8775F648430679A709E98d2b0Cb6250d2887EF";
    bat_token_->name = "Basic Attention Token";
    bat_token_->symbol = "BAT";
    bat_token_->is_erc20 = true;
    bat_token_->is_erc721 = false;
    bat_token_->decimals = 18;
    bat_token_->visible = true;
  }

  mojom::ERCTokenPtr GetToken1() { return token1_.Clone(); }
  mojom::ERCTokenPtr GetToken2() { return token2_.Clone(); }
  mojom::ERCTokenPtr GetEthToken() { return eth_token_.Clone(); }
  mojom::ERCTokenPtr GetBatToken() { return bat_token_.Clone(); }

  PrefService* GetPrefs() { return profile_->GetPrefs(); }
  ERCTokenRegistry* GetRegistry() { return ERCTokenRegistry::GetInstance(); }

  void GetUserAssets(const std::string& chain_id,
                     bool* callback_called,
                     std::vector<mojom::ERCTokenPtr>* out_tokens) {
    *callback_called = false;
    service_->GetUserAssets(
        chain_id,
        base::BindLambdaForTesting([&](std::vector<mojom::ERCTokenPtr> tokens) {
          *out_tokens = std::move(tokens);
          *callback_called = true;
        }));
    base::RunLoop().RunUntilIdle();
  }

  void AddUserAsset(mojom::ERCTokenPtr token,
                    const std::string& chain_id,
                    bool* callback_called,
                    bool* out_success) {
    *callback_called = false;
    service_->AddUserAsset(std::move(token), chain_id,
                           base::BindLambdaForTesting([&](bool success) {
                             *out_success = success;
                             *callback_called = true;
                           }));
    base::RunLoop().RunUntilIdle();
  }

  void RemoveUserAsset(const std::string& contract_address,
                       const std::string& chain_id,
                       bool* callback_called,
                       bool* out_success) {
    *callback_called = false;
    service_->RemoveUserAsset(contract_address, chain_id,
                              base::BindLambdaForTesting([&](bool success) {
                                *out_success = success;
                                *callback_called = true;
                              }));
    base::RunLoop().RunUntilIdle();
  }

  void SetUserAssetVisible(const std::string& contract_address,
                           const std::string& chain_id,
                           bool visible,
                           bool* callback_called,
                           bool* out_success) {
    *callback_called = false;
    service_->SetUserAssetVisible(contract_address, chain_id, visible,
                                  base::BindLambdaForTesting([&](bool success) {
                                    *out_success = success;
                                    *callback_called = true;
                                  }));
    base::RunLoop().RunUntilIdle();
  }

  void SetDefaultWallet(mojom::DefaultWallet default_wallet) {
    service_->SetDefaultWallet(default_wallet);
  }

  mojom::DefaultWallet GetDefaultWallet() {
    base::RunLoop run_loop;
    mojom::DefaultWallet default_wallet;
    service_->GetDefaultWallet(
        base::BindLambdaForTesting([&](mojom::DefaultWallet v) {
          default_wallet = v;
          run_loop.Quit();
        }));
    run_loop.Run();
    return default_wallet;
  }

  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<TestingProfile> profile_;
  std::unique_ptr<BraveWalletService> service_;
  mojom::ERCTokenPtr token1_;
  mojom::ERCTokenPtr token2_;
  mojom::ERCTokenPtr eth_token_;
  mojom::ERCTokenPtr bat_token_;
};

TEST_F(BraveWalletServiceUnitTest, GetUserAssets) {
  bool callback_called = false;
  bool success = false;
  std::vector<mojom::ERCTokenPtr> tokens;

  // Empty vector should be returned for invalid chain_id.
  GetUserAssets("", &callback_called, &tokens);
  EXPECT_TRUE(callback_called);
  EXPECT_EQ(tokens, std::vector<mojom::ERCTokenPtr>());

  GetUserAssets("0x123", &callback_called, &tokens);
  EXPECT_TRUE(callback_called);
  EXPECT_EQ(tokens, std::vector<mojom::ERCTokenPtr>());

  // Check mainnet default value.
  GetUserAssets("0x1", &callback_called, &tokens);
  EXPECT_TRUE(callback_called);
  EXPECT_EQ(tokens.size(), 2u);
  EXPECT_EQ(tokens[0], GetEthToken());
  EXPECT_EQ(tokens[1], GetBatToken());

  // Empty vector should be returned before any token is added.
  GetUserAssets("0x3", &callback_called, &tokens);
  EXPECT_TRUE(callback_called);
  EXPECT_EQ(tokens, std::vector<mojom::ERCTokenPtr>());

  // Prepare tokens to add.
  mojom::ERCTokenPtr token1 = GetToken1();
  mojom::ERCTokenPtr token2 = GetToken2();

  // Add tokens and test GetUserAsset.
  AddUserAsset(token1.Clone(), "0x1", &callback_called, &success);
  EXPECT_TRUE(callback_called);
  EXPECT_TRUE(success);

  // Adding token with lower case contract address should be converted to
  // checksum address..
  auto unchecked_token = token1.Clone();
  unchecked_token->contract_address =
      base::ToLowerASCII(unchecked_token->contract_address);
  AddUserAsset(std::move(unchecked_token), "0x4", &callback_called, &success);
  EXPECT_TRUE(callback_called);
  EXPECT_TRUE(success);

  AddUserAsset(token2.Clone(), "0x4", &callback_called, &success);
  EXPECT_TRUE(callback_called);
  EXPECT_TRUE(success);

  GetUserAssets("0x1", &callback_called, &tokens);
  EXPECT_TRUE(callback_called);
  EXPECT_EQ(tokens.size(), 3u);
  EXPECT_EQ(GetEthToken(), tokens[0]);
  EXPECT_EQ(GetBatToken(), tokens[1]);
  EXPECT_EQ(token1, tokens[2]);

  GetUserAssets("0x4", &callback_called, &tokens);
  EXPECT_TRUE(callback_called);
  EXPECT_EQ(tokens.size(), 2u);
  EXPECT_EQ(token1, tokens[0]);
  EXPECT_EQ(token2, tokens[1]);

  // Remove token1 from "0x1" and token2 from "0x4" and test GetUserAssets.
  RemoveUserAsset(token1->contract_address, "0x1", &callback_called, &success);
  EXPECT_TRUE(callback_called);
  EXPECT_TRUE(success);

  RemoveUserAsset(token2->contract_address, "0x4", &callback_called, &success);
  EXPECT_TRUE(callback_called);
  EXPECT_TRUE(success);

  GetUserAssets("0x1", &callback_called, &tokens);
  EXPECT_TRUE(callback_called);
  EXPECT_EQ(tokens.size(), 2u);
  EXPECT_EQ(tokens[0], GetEthToken());
  EXPECT_EQ(tokens[1], GetBatToken());

  GetUserAssets("0x4", &callback_called, &tokens);
  EXPECT_TRUE(callback_called);
  EXPECT_EQ(tokens.size(), 1u);
  EXPECT_EQ(token1, tokens[0]);
}

TEST_F(BraveWalletServiceUnitTest, AddUserAsset) {
  bool callback_called = false;
  bool success = false;
  std::vector<mojom::ERCTokenPtr> tokens;

  GetUserAssets("0x1", &callback_called, &tokens);
  EXPECT_TRUE(callback_called);
  EXPECT_EQ(tokens.size(), 2u);
  EXPECT_EQ(tokens[0], GetEthToken());
  EXPECT_EQ(tokens[1], GetBatToken());

  callback_called = false;
  mojom::ERCTokenPtr token;
  GetRegistry()->GetTokenByContract(
      "0x06012c8cf97BEaD5deAe237070F9587f8E7A266d",
      base::BindLambdaForTesting([&](mojom::ERCTokenPtr ercToken) {
        token = std::move(ercToken);
        callback_called = true;
      }));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
  ASSERT_EQ(token->symbol, "CK");

  // Token with empty contract address will fail.
  auto token_with_empty_contract_address = token.Clone();
  token_with_empty_contract_address->contract_address = "";
  AddUserAsset(std::move(token_with_empty_contract_address), "0x1",
               &callback_called, &success);
  EXPECT_TRUE(callback_called);
  EXPECT_FALSE(success);

  // Invalid chain_id will fail.
  AddUserAsset(token.Clone(), "0x123", &callback_called, &success);
  EXPECT_TRUE(callback_called);
  EXPECT_FALSE(success);

  // Add Crypto Kitties.
  AddUserAsset(token.Clone(), "0x1", &callback_called, &success);
  EXPECT_TRUE(callback_called);
  EXPECT_TRUE(success);

  // Check Crypto Kitties is added as expected.
  GetUserAssets("0x1", &callback_called, &tokens);
  EXPECT_TRUE(callback_called);
  EXPECT_EQ(tokens.size(), 3u);
  EXPECT_EQ(tokens[0], GetEthToken());
  EXPECT_EQ(tokens[1], GetBatToken());
  EXPECT_EQ(tokens[2], token);

  // Adding token with same address in the same chain will fail.
  AddUserAsset(token.Clone(), "0x1", &callback_called, &success);
  EXPECT_TRUE(callback_called);
  EXPECT_FALSE(success);

  // Adding token with same address in lower cases in the same chain will fail.
  auto token_with_unchecked_address = token.Clone();
  token_with_unchecked_address->contract_address =
      base::ToLowerASCII(token->contract_address);
  AddUserAsset(token_with_unchecked_address.Clone(), "0x1", &callback_called,
               &success);
  EXPECT_TRUE(callback_called);
  EXPECT_FALSE(success);

  // Adding token with same address in a different chain will succeed.
  // And the address will be converted to checksum address.
  GetUserAssets("0x4", &callback_called, &tokens);
  EXPECT_TRUE(callback_called);
  EXPECT_TRUE(tokens.empty());

  AddUserAsset(token_with_unchecked_address.Clone(), "0x4", &callback_called,
               &success);
  EXPECT_TRUE(callback_called);
  EXPECT_TRUE(success);

  GetUserAssets("0x4", &callback_called, &tokens);
  EXPECT_TRUE(callback_called);
  EXPECT_EQ(tokens.size(), 1u);
  EXPECT_EQ(tokens[0], token);
}

TEST_F(BraveWalletServiceUnitTest, RemoveUserAsset) {
  mojom::ERCTokenPtr token1 = GetToken1();
  mojom::ERCTokenPtr token2 = GetToken2();

  bool callback_called = false;
  bool success = false;
  std::vector<mojom::ERCTokenPtr> tokens;

  // Add tokens
  AddUserAsset(token1.Clone(), "0x1", &callback_called, &success);
  EXPECT_TRUE(callback_called);
  EXPECT_TRUE(success);

  AddUserAsset(token2.Clone(), "0x1", &callback_called, &success);
  EXPECT_TRUE(callback_called);
  EXPECT_TRUE(success);

  AddUserAsset(token2.Clone(), "0x4", &callback_called, &success);
  EXPECT_TRUE(callback_called);
  EXPECT_TRUE(success);

  GetUserAssets("0x1", &callback_called, &tokens);
  EXPECT_TRUE(callback_called);
  EXPECT_EQ(tokens.size(), 4u);
  EXPECT_EQ(tokens[0], GetEthToken());
  EXPECT_EQ(tokens[1], GetBatToken());
  EXPECT_EQ(tokens[2], token1);
  EXPECT_EQ(tokens[3], token2);

  GetUserAssets("0x4", &callback_called, &tokens);
  EXPECT_TRUE(callback_called);
  EXPECT_EQ(tokens.size(), 1u);
  EXPECT_EQ(tokens[0], token2);

  // Remove token with invalid contract_address returns false.
  RemoveUserAsset("", "0x1", &callback_called, &success);
  EXPECT_TRUE(callback_called);
  EXPECT_FALSE(success);

  // Remove token with invalid network_id returns false.
  RemoveUserAsset(token1->contract_address, "0x123", &callback_called,
                  &success);
  EXPECT_TRUE(callback_called);
  EXPECT_FALSE(success);

  // Returns false when we annot find the list with network_id.
  RemoveUserAsset(token1->contract_address, "0x3", &callback_called, &success);
  EXPECT_TRUE(callback_called);
  EXPECT_FALSE(success);

  // Remove non-exist token returns true.
  RemoveUserAsset(token1->contract_address, "0x4", &callback_called, &success);
  EXPECT_TRUE(callback_called);
  EXPECT_TRUE(success);

  // Remove existing token.
  RemoveUserAsset(token2->contract_address, "0x1", &callback_called, &success);
  EXPECT_TRUE(callback_called);
  EXPECT_TRUE(success);

  // Lowercase address will be converted to checksum address when removing
  // token.
  RemoveUserAsset(base::ToLowerASCII(GetBatToken()->contract_address), "0x1",
                  &callback_called, &success);
  EXPECT_TRUE(callback_called);
  EXPECT_TRUE(success);

  GetUserAssets("0x1", &callback_called, &tokens);
  EXPECT_TRUE(callback_called);
  EXPECT_EQ(tokens.size(), 2u);
  EXPECT_EQ(tokens[0], GetEthToken());
  EXPECT_EQ(tokens[1], token1);
}

TEST_F(BraveWalletServiceUnitTest, SetUserAssetVisible) {
  mojom::ERCTokenPtr token1 = GetToken1();
  mojom::ERCTokenPtr token2 = GetToken2();

  bool callback_called = false;
  bool success = false;
  std::vector<mojom::ERCTokenPtr> tokens;

  // Add tokens
  AddUserAsset(token1.Clone(), "0x1", &callback_called, &success);
  EXPECT_TRUE(callback_called);
  EXPECT_TRUE(success);

  AddUserAsset(token2.Clone(), "0x1", &callback_called, &success);
  EXPECT_TRUE(callback_called);
  EXPECT_TRUE(success);

  AddUserAsset(token2.Clone(), "0x4", &callback_called, &success);
  EXPECT_TRUE(callback_called);
  EXPECT_TRUE(success);

  GetUserAssets("0x1", &callback_called, &tokens);
  EXPECT_TRUE(callback_called);
  EXPECT_EQ(tokens.size(), 4u);
  EXPECT_EQ(tokens[0], GetEthToken());
  EXPECT_EQ(tokens[1], GetBatToken());
  EXPECT_EQ(tokens[2], token1);
  EXPECT_EQ(tokens[3], token2);

  GetUserAssets("0x4", &callback_called, &tokens);
  EXPECT_TRUE(callback_called);
  EXPECT_EQ(tokens.size(), 1u);
  EXPECT_EQ(tokens[0], token2);

  // Empty contract_address return false.
  SetUserAssetVisible("", "0x1", false, &callback_called, &success);
  EXPECT_TRUE(callback_called);
  EXPECT_FALSE(success);

  // Invalid chain_id return false.
  SetUserAssetVisible(token1->contract_address, "0x123", false,
                      &callback_called, &success);
  EXPECT_TRUE(callback_called);
  EXPECT_FALSE(success);

  // List for this network_id is not existed should return false.
  SetUserAssetVisible(token1->contract_address, "0x3", false, &callback_called,
                      &success);
  EXPECT_TRUE(callback_called);
  EXPECT_FALSE(success);

  // No entry with this contract address exists in the list.
  SetUserAssetVisible(token1->contract_address, "0x4", false, &callback_called,
                      &success);
  EXPECT_TRUE(callback_called);
  EXPECT_FALSE(success);

  // Set visible to false for BAT & token1 in "0x1" and token2 in "0x4".
  SetUserAssetVisible(token1->contract_address, "0x1", false, &callback_called,
                      &success);
  EXPECT_TRUE(callback_called);
  EXPECT_TRUE(success);

  // Lowercase address will be converted to checksum address directly.
  SetUserAssetVisible(base::ToLowerASCII(GetBatToken()->contract_address),
                      "0x1", false, &callback_called, &success);
  EXPECT_TRUE(callback_called);
  EXPECT_TRUE(success);

  SetUserAssetVisible(token2->contract_address, "0x4", false, &callback_called,
                      &success);
  EXPECT_TRUE(callback_called);
  EXPECT_TRUE(success);

  GetUserAssets("0x1", &callback_called, &tokens);
  EXPECT_TRUE(callback_called);
  EXPECT_EQ(tokens.size(), 4u);
  EXPECT_EQ(tokens[0]->contract_address, GetEthToken()->contract_address);
  EXPECT_TRUE(tokens[0]->visible);
  EXPECT_EQ(tokens[1]->contract_address, GetBatToken()->contract_address);
  EXPECT_FALSE(tokens[1]->visible);
  EXPECT_EQ(tokens[2]->contract_address, token1->contract_address);
  EXPECT_FALSE(tokens[2]->visible);
  EXPECT_EQ(tokens[3]->contract_address, token2->contract_address);
  EXPECT_TRUE(tokens[3]->visible);

  GetUserAssets("0x4", &callback_called, &tokens);
  EXPECT_TRUE(callback_called);
  EXPECT_EQ(tokens.size(), 1u);
  EXPECT_EQ(tokens[0]->contract_address, token2->contract_address);
  EXPECT_FALSE(tokens[0]->visible);
}

TEST_F(BraveWalletServiceUnitTest, GetChecksumAddress) {
  absl::optional<std::string> addr = service_->GetChecksumAddress(
      "0x06012c8cf97bead5deae237070f9587f8e7a266d", "0x1");
  EXPECT_EQ(addr.value(), "0x06012c8cf97BEaD5deAe237070F9587f8E7A266d");

  addr = service_->GetChecksumAddress(
      "0x06012c8cf97BEaD5deAe237070F9587f8E7A266d", "0x1");
  EXPECT_EQ(addr.value(), "0x06012c8cf97BEaD5deAe237070F9587f8E7A266d");

  addr = service_->GetChecksumAddress("", "0x1");
  EXPECT_FALSE(addr.has_value());

  addr = service_->GetChecksumAddress("0x123", "0x1");
  EXPECT_FALSE(addr.has_value());

  addr = service_->GetChecksumAddress("123", "0x1");
  EXPECT_FALSE(addr.has_value());

  addr = service_->GetChecksumAddress(
      "06012c8cf97BEaD5deAe237070F9587f8E7A266d", "0x1");
  EXPECT_FALSE(addr.has_value());
}

TEST_F(BraveWalletServiceUnitTest, GetAndSetDefaultWallet) {
  SetDefaultWallet(mojom::DefaultWallet::BraveWallet);
  EXPECT_EQ(GetDefaultWallet(), mojom::DefaultWallet::BraveWallet);

  SetDefaultWallet(mojom::DefaultWallet::CryptoWallets);
  EXPECT_EQ(GetDefaultWallet(), mojom::DefaultWallet::CryptoWallets);

  SetDefaultWallet(mojom::DefaultWallet::None);
  EXPECT_EQ(GetDefaultWallet(), mojom::DefaultWallet::None);

  SetDefaultWallet(mojom::DefaultWallet::Metamask);
  EXPECT_EQ(GetDefaultWallet(), mojom::DefaultWallet::Metamask);

  SetDefaultWallet(mojom::DefaultWallet::Ask);
  EXPECT_EQ(GetDefaultWallet(), mojom::DefaultWallet::Ask);
}

}  // namespace brave_wallet
