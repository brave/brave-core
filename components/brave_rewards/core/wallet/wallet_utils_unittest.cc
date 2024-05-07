/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/common/mojom/rewards.mojom.h"
#include "brave/components/brave_rewards/core/common/prefs.h"
#include "brave/components/brave_rewards/core/global_constants.h"
#include "brave/components/brave_rewards/core/test/rewards_engine_test.h"
#include "brave/components/brave_rewards/core/wallet/wallet_util.h"

using testing::Test;
using testing::Values;
using testing::WithParamInterface;

namespace brave_rewards::internal::wallet {

mojom::ExternalWalletPtr ExternalWalletPtrFromJSON(RewardsEngine& engine,
                                                   std::string wallet_string,
                                                   std::string wallet_type);

class RewardsWalletUtilTest : public RewardsEngineTest {};

TEST_F(RewardsWalletUtilTest, InvalidJSON) {
  EXPECT_FALSE(ExternalWalletPtrFromJSON(engine(), "", "uphold"));
}

TEST_F(RewardsWalletUtilTest, ExternalWalletPtrFromJSON) {
  const char data[] =
      "{\n"
      "  \"token\": \"sI5rKiy6ijzbbJgE2MMFzAbTc6udYYXEi3wzS9iknP6n\",\n"
      "  \"address\": \"6a752063-8958-44d5-b5db-71543f18567d\",\n"
      "  \"status\": 2,\n"
      "  \"user_name\": \"random_user\",\n"
      "  \"fees\": {\"brave\": 5.00}"
      "}\n";

  mojom::ExternalWalletPtr wallet =
      ExternalWalletPtrFromJSON(engine(), data, "uphold");
  EXPECT_EQ(wallet->token, "sI5rKiy6ijzbbJgE2MMFzAbTc6udYYXEi3wzS9iknP6n");
  EXPECT_EQ(wallet->address, "6a752063-8958-44d5-b5db-71543f18567d");
  EXPECT_EQ(wallet->status, mojom::WalletStatus::kConnected);
  EXPECT_EQ(wallet->user_name, "random_user");
  EXPECT_NE(wallet->fees.find("brave"), wallet->fees.end());
  EXPECT_EQ(wallet->fees["brave"], 5.00);
}

using TransitionWalletCreateParamType =
    std::tuple<std::string,          // test name suffix
               mojom::WalletStatus,  // "to" WalletStatus
               bool,                 // wallet already exists
               bool                  // expected outcome
               >;

class TransitionWalletCreate
    : public RewardsEngineTest,
      public WithParamInterface<TransitionWalletCreateParamType> {};

TEST_P(TransitionWalletCreate, Paths) {
  const auto& [ignore, to, wallet_already_exists, expected] = GetParam();

  engine().Get<Prefs>().SetString(
      prefs::kWalletUphold,
      wallet_already_exists ? FakeEncryption::Base64EncryptString("{}") : "");

  const auto wallet = TransitionWallet(engine(), constant::kWalletUphold, to);

  EXPECT_EQ(static_cast<bool>(wallet), expected);

  if (wallet) {
    EXPECT_EQ(wallet->type, constant::kWalletUphold);
    EXPECT_EQ(wallet->status, to);
    EXPECT_EQ(wallet->account_url.empty(),
              to == mojom::WalletStatus::kNotConnected);
    EXPECT_EQ(wallet->activity_url.empty(), wallet->address.empty());

    EXPECT_TRUE(wallet->token.empty());
    EXPECT_TRUE(wallet->address.empty());
  }
}

INSTANTIATE_TEST_SUITE_P(
    RewardsWalletUtilTest,
    TransitionWalletCreate,
    Values(TransitionWalletCreateParamType{"wallet_already_exists",
                                           mojom::WalletStatus::kNotConnected,
                                           true, false},
           TransitionWalletCreateParamType{
               "attempting_to_create_wallet_as_kConnected",
               mojom::WalletStatus::kConnected, false, false},
           TransitionWalletCreateParamType{
               "attempting_to_create_wallet_as_kLoggedOut",
               mojom::WalletStatus::kLoggedOut, false, false},
           TransitionWalletCreateParamType{"create_success",
                                           mojom::WalletStatus::kNotConnected,
                                           false, true}),
    [](const auto& info) { return std::get<0>(info.param); });

using TransitionWalletTransitionParamType =
    std::tuple<std::string,  // test name suffix
               base::RepeatingCallback<mojom::ExternalWalletPtr()>,
               mojom::WalletStatus,  // "to" WalletStatus
               bool                  // expected outcome
               >;

class TransitionWalletTransition
    : public RewardsEngineTest,
      public WithParamInterface<TransitionWalletTransitionParamType> {};

TEST_P(TransitionWalletTransition, Paths) {
  const auto& [ignore, make_from_wallet, to, expected] = GetParam();

  const auto to_wallet = TransitionWallet(engine(), make_from_wallet.Run(), to);

  EXPECT_EQ(static_cast<bool>(to_wallet), expected);

  if (to_wallet) {
    EXPECT_EQ(to_wallet->type, constant::kWalletUphold);
    EXPECT_EQ(to_wallet->status, to);

    EXPECT_EQ(to_wallet->account_url.empty(),
              to == mojom::WalletStatus::kNotConnected);
    EXPECT_EQ(to_wallet->activity_url.empty(), to_wallet->address.empty());

    if (to != mojom::WalletStatus::kConnected) {
      EXPECT_TRUE(to == mojom::WalletStatus::kNotConnected ||
                  to == mojom::WalletStatus::kLoggedOut);

      EXPECT_TRUE(to_wallet->activity_url.empty());

      EXPECT_TRUE(to_wallet->token.empty());
      EXPECT_TRUE(to_wallet->address.empty());
    }
  }
}

INSTANTIATE_TEST_SUITE_P(
    RewardsWalletUtilTest,
    TransitionWalletTransition,
    Values(
        TransitionWalletTransitionParamType{
            "kNotConnected__kNotConnected", base::BindRepeating([] {
              auto wallet = mojom::ExternalWallet::New();
              wallet->status = mojom::WalletStatus::kNotConnected;
              return wallet;
            }),
            mojom::WalletStatus::kNotConnected, false},
        TransitionWalletTransitionParamType{
            "kNotConnected__kLoggedOut", base::BindRepeating([] {
              auto wallet = mojom::ExternalWallet::New();
              wallet->status = mojom::WalletStatus::kNotConnected;
              return wallet;
            }),
            mojom::WalletStatus::kLoggedOut, false},
        TransitionWalletTransitionParamType{
            "kNotConnected__kConnected_no_token", base::BindRepeating([] {
              auto wallet = mojom::ExternalWallet::New();
              wallet->status = mojom::WalletStatus::kNotConnected;
              wallet->address = "address";
              return wallet;
            }),
            mojom::WalletStatus::kConnected, false},
        TransitionWalletTransitionParamType{
            "kNotConnected__kConnected_no_address", base::BindRepeating([] {
              auto wallet = mojom::ExternalWallet::New();
              wallet->status = mojom::WalletStatus::kNotConnected;
              wallet->token = "token";
              return wallet;
            }),
            mojom::WalletStatus::kConnected, false},
        TransitionWalletTransitionParamType{
            "kNotConnected__kConnected", base::BindRepeating([] {
              auto wallet = mojom::ExternalWallet::New();
              wallet->type = constant::kWalletUphold;
              wallet->status = mojom::WalletStatus::kNotConnected;
              wallet->address = "address";
              wallet->token = "token";
              return wallet;
            }),
            mojom::WalletStatus::kConnected, true},
        TransitionWalletTransitionParamType{
            "kLoggedOut__kNotConnected", base::BindRepeating([] {
              auto wallet = mojom::ExternalWallet::New();
              wallet->type = constant::kWalletUphold;
              wallet->status = mojom::WalletStatus::kLoggedOut;
              return wallet;
            }),
            mojom::WalletStatus::kNotConnected, true},
        TransitionWalletTransitionParamType{
            "kLoggedOut__kLoggedOut", base::BindRepeating([] {
              auto wallet = mojom::ExternalWallet::New();
              wallet->status = mojom::WalletStatus::kLoggedOut;
              return wallet;
            }),
            mojom::WalletStatus::kLoggedOut, false},
        TransitionWalletTransitionParamType{
            "kLoggedOut__kConnected_no_token", base::BindRepeating([] {
              auto wallet = mojom::ExternalWallet::New();
              wallet->status = mojom::WalletStatus::kLoggedOut;
              wallet->address = "address";
              return wallet;
            }),
            mojom::WalletStatus::kConnected, false},
        TransitionWalletTransitionParamType{
            "kLoggedOut__kConnected_no_address", base::BindRepeating([] {
              auto wallet = mojom::ExternalWallet::New();
              wallet->status = mojom::WalletStatus::kLoggedOut;
              wallet->token = "token";
              return wallet;
            }),
            mojom::WalletStatus::kConnected, false},
        TransitionWalletTransitionParamType{
            "kLoggedOut__kConnected", base::BindRepeating([] {
              auto wallet = mojom::ExternalWallet::New();
              wallet->type = constant::kWalletUphold;
              wallet->status = mojom::WalletStatus::kLoggedOut;
              wallet->address = "address";
              wallet->token = "token";
              return wallet;
            }),
            mojom::WalletStatus::kConnected, true},
        TransitionWalletTransitionParamType{
            "kConnected__kNotConnected", base::BindRepeating([] {
              auto wallet = mojom::ExternalWallet::New();
              wallet->type = constant::kWalletUphold;
              wallet->status = mojom::WalletStatus::kConnected;
              return wallet;
            }),
            mojom::WalletStatus::kNotConnected, true},
        TransitionWalletTransitionParamType{
            "kConnected__kLoggedOut", base::BindRepeating([] {
              auto wallet = mojom::ExternalWallet::New();
              wallet->type = constant::kWalletUphold;
              wallet->status = mojom::WalletStatus::kConnected;
              return wallet;
            }),
            mojom::WalletStatus::kLoggedOut, true},
        TransitionWalletTransitionParamType{
            "kConnected__kConnected", base::BindRepeating([] {
              auto wallet = mojom::ExternalWallet::New();
              wallet->status = mojom::WalletStatus::kConnected;
              return wallet;
            }),
            mojom::WalletStatus::kConnected, false}),
    [](const auto& info) {
      return (std::get<3>(info.param) ? "" : "in") +
             std::string("valid_transition_") + std::get<0>(info.param);
    });

}  // namespace brave_rewards::internal::wallet
