/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/wallet_provider/solana/solana_wallet_provider.h"

#include <cmath>
#include <utility>
#include <vector>

#include "base/base64url.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_rewards/core/common/environment_config.h"
#include "brave/components/brave_rewards/core/common/signer.h"
#include "brave/components/brave_rewards/core/database/database.h"
#include "brave/components/brave_rewards/core/global_constants.h"
#include "brave/components/brave_rewards/core/logging/event_log_keys.h"
#include "brave/components/brave_rewards/core/state/state_keys.h"
#include "brave/components/brave_rewards/core/wallet/wallet.h"
#include "brave/components/brave_rewards/core/wallet/wallet_util.h"
#include "brave/components/brave_rewards/core/wallet_provider/linkage_checker.h"
#include "net/base/url_util.h"

namespace brave_rewards::internal {

namespace {

constexpr char kSplBatTokenMint[] =
    "EPeUFDgHRxs9xxEPVaL6kfGQvCon7jmAWKVUHuux1Tpz";
constexpr base::TimeDelta kPollingInterval = base::Seconds(10);
constexpr base::TimeDelta kPollingTimeout = base::Minutes(5);

std::string UsernameFromAddress(const std::string& address) {
  size_t length = address.length();
  if (length < 10) {
    return address;
  }
  std::string_view address_view(address);
  return base::StrCat({address_view.substr(0, 5), "...",
                       address_view.substr(length - 4, length)});
}

}  // namespace

SolanaWalletProvider::SolanaWalletProvider(RewardsEngine& engine)
    : RewardsEngineHelper(engine), WalletProvider(engine) {}

SolanaWalletProvider::~SolanaWalletProvider() = default;

const char* SolanaWalletProvider::WalletType() const {
  return constant::kWalletSolana;
}

void SolanaWalletProvider::AssignWalletLinks(
    mojom::ExternalWallet& external_wallet) {
  auto explorer_url =
      GURL("https://explorer.solana.com/address/")
          .Resolve(base::StrCat({external_wallet.address, "/tokens"}));
  external_wallet.account_url = explorer_url.spec();
  external_wallet.activity_url = explorer_url.spec();
}

void SolanaWalletProvider::FetchBalance(
    base::OnceCallback<void(mojom::Result, double)> callback) {
  auto wallet = GetWalletIf({mojom::WalletStatus::kConnected});
  if (!wallet) {
    return std::move(callback).Run(mojom::Result::FAILED, 0.0);
  }

  client().GetSPLTokenAccountBalance(
      wallet->address, kSplBatTokenMint,
      base::BindOnce(&SolanaWalletProvider::OnAccountBalanceFetched,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void SolanaWalletProvider::BeginLogin(
    BeginExternalWalletLoginCallback callback) {
  Get<endpoints::PostChallenges>().Request(
      base::BindOnce(&SolanaWalletProvider::OnPostChallengesResponse,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void SolanaWalletProvider::OnPostChallengesResponse(
    BeginExternalWalletLoginCallback callback,
    endpoints::PostChallenges::Result result) {
  if (!result.has_value()) {
    std::move(callback).Run(nullptr);
    return;
  }

  const std::string& challenge_id = result.value();
  DCHECK(!challenge_id.empty());

  auto wallet = engine().wallet()->GetWallet();
  if (!wallet) {
    LogError(FROM_HERE) << "Rewards wallet is empty";
    std::move(callback).Run(nullptr);
    return;
  }

  auto signer = Signer::FromRecoverySeed(wallet->recovery_seed);
  if (!signer) {
    LogError(FROM_HERE) << "Unable to sign message";
    std::move(callback).Run(nullptr);
    return;
  }

  std::string message =
      base::StrCat({base::ToLowerASCII(wallet->payment_id), ".", challenge_id});
  auto signed_message = signer->SignMessage(base::as_byte_span(message));
  std::string signature;
  base::Base64UrlEncode(
      signed_message, base::Base64UrlEncodePolicy::INCLUDE_PADDING, &signature);

  auto url = Get<EnvironmentConfig>().rewards_url().Resolve("/connect/");
  url = net::AppendOrReplaceQueryParameter(url, "msg", message);
  url = net::AppendOrReplaceQueryParameter(url, "sig", signature);

  auto params = mojom::ExternalWalletLoginParams::New();
  params->url = url.spec();
  params->cookies["__Secure-CSRF_TOKEN"] = challenge_id;
  std::move(callback).Run(std::move(params));

  polling_timer_.Start(FROM_HERE, kPollingInterval, this,
                       &SolanaWalletProvider::PollWalletStatus);

  polling_timeout_.Start(FROM_HERE, kPollingTimeout, this,
                         &SolanaWalletProvider::OnPollingTimeout);
}

void SolanaWalletProvider::OnAccountBalanceFetched(
    base::OnceCallback<void(mojom::Result, double)> callback,
    mojom::SolanaAccountBalancePtr balance) {
  if (!balance) {
    LogError(FROM_HERE) << "Unable to retrieve Solana account balance";
    std::move(callback).Run(mojom::Result::FAILED, 0);
    return;
  }

  uint64_t amount_value = 0;
  if (!base::StringToUint64(balance->amount, &amount_value)) {
    LogError(FROM_HERE) << "Unable to parse Solana account balance";
    std::move(callback).Run(mojom::Result::FAILED, 0);
    return;
  }

  auto denominator = std::pow<double>(10, balance->decimals);

  std::move(callback).Run(mojom::Result::OK,
                          static_cast<double>(amount_value) / denominator);
}

void SolanaWalletProvider::PollWalletStatus() {
  Get<LinkageChecker>().CheckLinkage();
}

void SolanaWalletProvider::OnPollingTimeout() {
  polling_timer_.Stop();
}

std::string SolanaWalletProvider::GetFeeAddress() const {
  return "";
}

void SolanaWalletProvider::OnWalletLinked(const std::string& address) {
  DCHECK(!address.empty());

  polling_timer_.Stop();
  polling_timeout_.Stop();

  wallet::MaybeCreateWallet(engine(), WalletType());
  auto wallet = GetWalletIf({mojom::WalletStatus::kNotConnected});
  if (!wallet) {
    return;
  }

  // Current external wallet invariants require both an address and a token for
  // connected wallets. Use an arbitrary non-empty value for the token.
  wallet->address = address;
  wallet->token = address;
  wallet->user_name = UsernameFromAddress(address);

  if (!wallet::TransitionWallet(engine(), std::move(wallet),
                                mojom::WalletStatus::kConnected)) {
    LogError(FROM_HERE) << "Failed to transition " << WalletType()
                        << " wallet state";
    return;
  }

  engine().SetState(state::kExternalWalletType, std::string(WalletType()));
  engine().SetState(state::kAutoContributeEnabled, false);
  client().ExternalWalletConnected();
  engine().database()->SaveEventLog(
      log::kWalletVerified,
      WalletType() + std::string("/") + address.substr(0, 5));
}

}  // namespace brave_rewards::internal
