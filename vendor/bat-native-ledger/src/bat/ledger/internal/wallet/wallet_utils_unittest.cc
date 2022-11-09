/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/wallet/wallet_util.h"

#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/core/bat_ledger_test.h"
#include "bat/ledger/internal/state/state_keys.h"
#include "bat/ledger/mojom_structs.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=*WalletUtilTest*

using testing::Values;
using testing::WithParamInterface;

namespace ledger::wallet {

mojom::ExternalWalletPtr ExternalWalletPtrFromJSON(std::string wallet_string,
                                                   std::string wallet_type);

class WalletUtilTest : public BATLedgerTest {};

TEST_F(WalletUtilTest, InvalidJSON) {
  const char data[] = "";
  mojom::ExternalWalletPtr wallet = ExternalWalletPtrFromJSON(data, "uphold");
  EXPECT_EQ(nullptr, wallet.get());
}

TEST_F(WalletUtilTest, ExternalWalletPtrFromJSON) {
  const char data[] =
      "{\n"
      "  \"token\": \"sI5rKiy6ijzbbJgE2MMFzAbTc6udYYXEi3wzS9iknP6n\",\n"
      "  \"address\": \"6a752063-8958-44d5-b5db-71543f18567d\",\n"
      "  \"one_time_string\": \"eda4c873eac72e1ecc30e77b25bb623b8b5bf99f\",\n"
      "  \"status\": 2,\n"
      "  \"user_name\": \"random_user\",\n"
      "  \"add_url\": \"https://random.domain/add\","
      "  \"withdraw_url\": \"https://random.domain/withdraw\","
      "  \"account_url\": \"https://random.domain/account\","
      "  \"login_url\": \"https://random.domain/login\","
      "  \"fees\": {\"brave\": 5.00}"
      "}\n";

  mojom::ExternalWalletPtr wallet = ExternalWalletPtrFromJSON(data, "uphold");
  EXPECT_EQ(wallet->token, "sI5rKiy6ijzbbJgE2MMFzAbTc6udYYXEi3wzS9iknP6n");
  EXPECT_EQ(wallet->address, "6a752063-8958-44d5-b5db-71543f18567d");
  EXPECT_EQ(wallet->one_time_string,
            "eda4c873eac72e1ecc30e77b25bb623b8b5bf99f");
  EXPECT_EQ(wallet->status, ledger::mojom::WalletStatus::kConnected);
  EXPECT_EQ(wallet->user_name, "random_user");
  EXPECT_EQ(wallet->add_url, "https://random.domain/add");
  EXPECT_EQ(wallet->withdraw_url, "https://random.domain/withdraw");
  EXPECT_EQ(wallet->account_url, "https://random.domain/account");
  EXPECT_EQ(wallet->login_url, "https://random.domain/login");
  EXPECT_NE(wallet->fees.find("brave"), wallet->fees.end());
  EXPECT_EQ(wallet->fees["brave"], 5.00);
}

using TransitionWalletCreateOrResetParamType =
    std::tuple<std::string,          // test name suffix
               mojom::WalletStatus,  // "to" WalletStatus
               bool,                 // create wallet
               bool                  // expected outcome
               >;

class TransitionWalletCreateOrReset
    : public BATLedgerTest,
      public WithParamInterface<TransitionWalletCreateOrResetParamType> {};

TEST_P(TransitionWalletCreateOrReset, Paths) {
  const auto& [ignore, to, create, expected] = GetParam();

  if (!create) {
    GetTestLedgerClient()->SetStringState(
        state::kWalletUphold, FakeEncryption::Base64EncryptString("{}"));
  }

  const auto wallet =
      TransitionWallet(GetLedgerImpl(), constant::kWalletUphold, to);
  EXPECT_EQ(static_cast<bool>(wallet), expected);

  if (wallet) {
    EXPECT_EQ(wallet->type, constant::kWalletUphold);
    EXPECT_FALSE(wallet->one_time_string.empty());
    EXPECT_FALSE(wallet->code_verifier.empty());
    EXPECT_EQ(wallet->status, to);
    EXPECT_FALSE(wallet->account_url.empty());
    EXPECT_TRUE(wallet->activity_url.empty());
    EXPECT_TRUE(wallet->add_url.empty());
    EXPECT_FALSE(wallet->login_url.empty());
    EXPECT_TRUE(wallet->withdraw_url.empty());

    EXPECT_TRUE(wallet->token.empty());
    EXPECT_TRUE(wallet->address.empty());
  }
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(
  WalletUtilTest,
  TransitionWalletCreateOrReset,
  Values(
    TransitionWalletCreateOrResetParamType{
      "attempting_to_create_wallet_as_kConnected",
      mojom::WalletStatus::kConnected,
      true,
      false
    },
    TransitionWalletCreateOrResetParamType{
      "attempting_to_create_wallet_as_kLoggedOut",
      mojom::WalletStatus::kLoggedOut,
      true,
      false
    },
    TransitionWalletCreateOrResetParamType{
      "create_success",
      mojom::WalletStatus::kNotConnected,
      true,
      true
    },
    TransitionWalletCreateOrResetParamType{
      "attempting_to_reset_wallet_to_kConnected",
      mojom::WalletStatus::kConnected,
      false,
      false
    },
    TransitionWalletCreateOrResetParamType{
      "reset_success_kNotConnected",
      mojom::WalletStatus::kNotConnected,
      false,
      true
    },
    TransitionWalletCreateOrResetParamType{
      "reset_success_kLoggedOut",
      mojom::WalletStatus::kLoggedOut,
      false,
      true
    }
  ),
  [](const auto& info) {
    return std::get<0>(info.param);
  }
);
// clang-format on

using TransitionWalletTransitionParamType =
    std::tuple<std::string,                                // test name suffix
               std::shared_ptr<mojom::ExternalWalletPtr>,  // from wallet
               mojom::WalletStatus,                        // "to" WalletStatus
               bool                                        // expected outcome
               >;

class TransitionWalletTransition
    : public BATLedgerTest,
      public WithParamInterface<TransitionWalletTransitionParamType> {};

TEST_P(TransitionWalletTransition, Paths) {
  const auto& [ignore, from_wallet, to, expected] = GetParam();

  const auto to_wallet =
      TransitionWallet(GetLedgerImpl(), std::move(*from_wallet), to);
  EXPECT_EQ(static_cast<bool>(to_wallet), expected);

  if (to_wallet) {
    EXPECT_EQ(to_wallet->type, constant::kWalletUphold);
    EXPECT_TRUE(to_wallet->one_time_string.empty());
    EXPECT_TRUE(to_wallet->code_verifier.empty());
    EXPECT_EQ(to_wallet->status, to);
    EXPECT_FALSE(to_wallet->account_url.empty());
    EXPECT_FALSE(to_wallet->login_url.empty());

    if (to == mojom::WalletStatus::kConnected) {
      EXPECT_FALSE(to_wallet->activity_url.empty());
      EXPECT_FALSE(to_wallet->add_url.empty());
      EXPECT_FALSE(to_wallet->withdraw_url.empty());
    } else {
      EXPECT_TRUE(to_wallet->activity_url.empty());
      EXPECT_TRUE(to_wallet->add_url.empty());
      EXPECT_TRUE(to_wallet->withdraw_url.empty());

      EXPECT_TRUE(to_wallet->token.empty());
      EXPECT_TRUE(to_wallet->address.empty());
    }
  }
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(
  WalletUtilTest,
  TransitionWalletTransition,
  Values(
    TransitionWalletTransitionParamType{
      "kNotConnected__kNotConnected",
      []{
        auto wallet = mojom::ExternalWallet::New();
        wallet->status = mojom::WalletStatus::kNotConnected;
        return std::make_shared<mojom::ExternalWalletPtr>(std::move(wallet));
      }(),
      mojom::WalletStatus::kNotConnected,
      false
    },
    TransitionWalletTransitionParamType{
      "kNotConnected__kLoggedOut",
      []{
        auto wallet = mojom::ExternalWallet::New();
        wallet->status = mojom::WalletStatus::kNotConnected;
        return std::make_shared<mojom::ExternalWalletPtr>(std::move(wallet));
      }(),
      mojom::WalletStatus::kLoggedOut,
      false
    },
    TransitionWalletTransitionParamType{
      "kNotConnected__kConnected_no_token",
      []{
        auto wallet = mojom::ExternalWallet::New();
        wallet->status = mojom::WalletStatus::kNotConnected;
        wallet->address = "address";
        return std::make_shared<mojom::ExternalWalletPtr>(std::move(wallet));
      }(),
      mojom::WalletStatus::kConnected,
      false
    },
    TransitionWalletTransitionParamType{
      "kNotConnected__kConnected_no_address",
      []{
        auto wallet = mojom::ExternalWallet::New();
        wallet->status = mojom::WalletStatus::kNotConnected;
        wallet->token = "token";
        return std::make_shared<mojom::ExternalWalletPtr>(std::move(wallet));
      }(),
      mojom::WalletStatus::kConnected,
      false
    },
    TransitionWalletTransitionParamType{
      "kNotConnected__kConnected",
      []{
        auto wallet = mojom::ExternalWallet::New();
        wallet->type = constant::kWalletUphold;
        wallet->status = mojom::WalletStatus::kNotConnected;
        wallet->address = "address";
        wallet->token = "token";
        return std::make_shared<mojom::ExternalWalletPtr>(std::move(wallet));
      }(),
      mojom::WalletStatus::kConnected,
      true
    },
    TransitionWalletTransitionParamType{
      "kLoggedOut__kNotConnected",
      []{
        auto wallet = mojom::ExternalWallet::New();
        wallet->type = constant::kWalletUphold;
        wallet->status = mojom::WalletStatus::kLoggedOut;
        return std::make_shared<mojom::ExternalWalletPtr>(std::move(wallet));
      }(),
      mojom::WalletStatus::kNotConnected,
      true
    },
    TransitionWalletTransitionParamType{
      "kLoggedOut__kLoggedOut",
      []{
        auto wallet = mojom::ExternalWallet::New();
        wallet->status = mojom::WalletStatus::kLoggedOut;
        return std::make_shared<mojom::ExternalWalletPtr>(std::move(wallet));
      }(),
      mojom::WalletStatus::kLoggedOut,
      false
    },
    TransitionWalletTransitionParamType{
      "kLoggedOut__kConnected_no_token",
      []{
        auto wallet = mojom::ExternalWallet::New();
        wallet->status = mojom::WalletStatus::kLoggedOut;
        wallet->address = "address";
        return std::make_shared<mojom::ExternalWalletPtr>(std::move(wallet));
      }(),
      mojom::WalletStatus::kConnected,
      false
    },
    TransitionWalletTransitionParamType{
      "kLoggedOut__kConnected_no_address",
      []{
        auto wallet = mojom::ExternalWallet::New();
        wallet->status = mojom::WalletStatus::kLoggedOut;
        wallet->token = "token";
        return std::make_shared<mojom::ExternalWalletPtr>(std::move(wallet));
      }(),
      mojom::WalletStatus::kConnected,
      false
    },
    TransitionWalletTransitionParamType{
      "kLoggedOut__kConnected",
      []{
        auto wallet = mojom::ExternalWallet::New();
        wallet->type = constant::kWalletUphold;
        wallet->status = mojom::WalletStatus::kLoggedOut;
        wallet->address = "address";
        wallet->token = "token";
        return std::make_shared<mojom::ExternalWalletPtr>(std::move(wallet));
      }(),
      mojom::WalletStatus::kConnected,
      true
    },
    TransitionWalletTransitionParamType{
      "kConnected__kNotConnected",
      []{
        auto wallet = mojom::ExternalWallet::New();
        wallet->type = constant::kWalletUphold;
        wallet->status = mojom::WalletStatus::kConnected;
        return std::make_shared<mojom::ExternalWalletPtr>(std::move(wallet));
      }(),
      mojom::WalletStatus::kNotConnected,
      true
    },
    TransitionWalletTransitionParamType{
      "kConnected__kLoggedOut",
      []{
        auto wallet = mojom::ExternalWallet::New();
        wallet->type = constant::kWalletUphold;
        wallet->status = mojom::WalletStatus::kConnected;
        return std::make_shared<mojom::ExternalWalletPtr>(std::move(wallet));
      }(),
      mojom::WalletStatus::kLoggedOut,
      true
    },
    TransitionWalletTransitionParamType{
      "kConnected__kConnected",
      []{
        auto wallet = mojom::ExternalWallet::New();
        wallet->status = mojom::WalletStatus::kConnected;
        return std::make_shared<mojom::ExternalWalletPtr>(std::move(wallet));
      }(),
      mojom::WalletStatus::kConnected,
      false
    }
  ),
  [](const auto& info) {
    return (std::get<3>(info.param) ? "" : "in") +
           std::string("valid_transition_") + std::get<0>(info.param);
  }
);
// clang-format on

}  // namespace ledger::wallet
