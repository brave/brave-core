/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/wallet/wallet.h"

#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/values.h"
#include "brave/components/brave_rewards/core/database/database.h"
#include "brave/components/brave_rewards/core/global_constants.h"
#include "brave/components/brave_rewards/core/logging/event_log_keys.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"
#include "brave/components/brave_rewards/core/state/state_keys.h"
#include "brave/components/brave_rewards/core/wallet/wallet_util.h"

#include "wally_bip39.h"  // NOLINT

namespace brave_rewards::internal {
namespace wallet {

Wallet::Wallet(RewardsEngineImpl& engine)
    : engine_(engine),
      create_(engine),
      balance_(engine),
      promotion_server_(engine) {}

Wallet::~Wallet() = default;

void Wallet::CreateWalletIfNecessary(absl::optional<std::string>&& geo_country,
                                     CreateRewardsWalletCallback callback) {
  create_.CreateWallet(std::move(geo_country), std::move(callback));
}

void Wallet::FetchBalance(FetchBalanceCallback callback) {
  balance_.Fetch(std::move(callback));
}

mojom::RewardsWalletPtr Wallet::GetWallet(bool* corrupted) {
  DCHECK(corrupted);
  *corrupted = false;

  const std::string json = engine_->GetState<std::string>(state::kWalletBrave);

  if (json.empty()) {
    return nullptr;
  }

  absl::optional<base::Value> value = base::JSONReader::Read(json);
  if (!value || !value->is_dict()) {
    BLOG(0, "Parsing of brave wallet failed");
    *corrupted = true;
    return nullptr;
  }

  auto wallet = mojom::RewardsWallet::New();

  const base::Value::Dict& dict = value->GetDict();
  const auto* payment_id = dict.FindString("payment_id");
  if (!payment_id) {
    *corrupted = true;
    return nullptr;
  }
  wallet->payment_id = *payment_id;

  const auto* seed = dict.FindString("recovery_seed");
  if (!seed || seed->empty()) {
    *corrupted = true;
    return nullptr;
  }
  std::string decoded_seed;
  if (!base::Base64Decode(*seed, &decoded_seed)) {
    BLOG(0, "Problem decoding recovery seed");
    *corrupted = true;
    return nullptr;
  }

  std::vector<uint8_t> vector_seed;
  vector_seed.assign(decoded_seed.begin(), decoded_seed.end());
  wallet->recovery_seed = vector_seed;

  return wallet;
}

mojom::RewardsWalletPtr Wallet::GetWallet() {
  bool corrupted;
  return GetWallet(&corrupted);
}

bool Wallet::SetWallet(mojom::RewardsWalletPtr wallet) {
  if (!wallet) {
    BLOG(0, "Rewards wallet is null!");
    return false;
  }

  const std::string seed_string = base::Base64Encode(wallet->recovery_seed);
  std::string event_string;
  if (wallet->recovery_seed.size() > 1) {
    event_string =
        std::to_string(wallet->recovery_seed[0] + wallet->recovery_seed[1]);
  }

  base::Value::Dict new_wallet;
  new_wallet.Set("payment_id", wallet->payment_id);
  new_wallet.Set("recovery_seed", seed_string);

  std::string json;
  base::JSONWriter::Write(new_wallet, &json);

  engine_->SetState(state::kWalletBrave, std::move(json));

  engine_->database()->SaveEventLog(state::kRecoverySeed, event_string);

  if (!wallet->payment_id.empty()) {
    engine_->database()->SaveEventLog(state::kPaymentId, wallet->payment_id);
  }

  return true;
}

}  // namespace wallet
}  // namespace brave_rewards::internal
