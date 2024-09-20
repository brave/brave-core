/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/wallet_provider/gemini/connect_gemini_wallet.h"

#include <utility>

#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_rewards/core/common/environment_config.h"
#include "brave/components/brave_rewards/core/common/random_util.h"
#include "brave/components/brave_rewards/core/common/url_helpers.h"
#include "brave/components/brave_rewards/core/database/database.h"
#include "brave/components/brave_rewards/core/endpoint/gemini/gemini_server.h"
#include "brave/components/brave_rewards/core/endpoints/brave/post_connect_gemini.h"
#include "brave/components/brave_rewards/core/endpoints/request_for.h"
#include "brave/components/brave_rewards/core/gemini/gemini.h"
#include "brave/components/brave_rewards/core/global_constants.h"
#include "brave/components/brave_rewards/core/logging/event_log_keys.h"
#include "brave/components/brave_rewards/core/rewards_engine.h"
#include "brave/components/brave_rewards/core/wallet/wallet.h"
#include "crypto/sha2.h"

namespace brave_rewards::internal {

using endpoints::GetRecipientIDGemini;
using endpoints::PostConnectGemini;
using endpoints::RequestFor;
using mojom::ConnectExternalWalletResult;
using wallet_provider::ConnectExternalWallet;

namespace gemini {

ConnectGeminiWallet::ConnectGeminiWallet(RewardsEngine& engine)
    : ConnectExternalWallet(engine), gemini_server_(engine) {}

ConnectGeminiWallet::~ConnectGeminiWallet() = default;

const char* ConnectGeminiWallet::WalletType() const {
  return constant::kWalletGemini;
}

std::string ConnectGeminiWallet::GetOAuthLoginURL() const {
  auto& config = engine_->Get<EnvironmentConfig>();

  auto url = config.gemini_oauth_url().Resolve("/auth");

  url = AppendOrReplaceQueryParameters(
      url, {{"client_id", config.gemini_client_id()},
            {"scope",
             "balances:read,"
             "history:read,"
             "crypto:send,"
             "account:read,"
             "payments:create,"
             "payments:send,"},
            {"redirect_uri", "rewards://gemini/authorization"},
            {"state", oauth_info_.one_time_string},
            {"response_type", "code"}});

  return url.spec();
}

void ConnectGeminiWallet::Authorize(ConnectExternalWalletCallback callback) {
  DCHECK(!oauth_info_.code.empty());

  const auto rewards_wallet = engine_->wallet()->GetWallet();
  if (!rewards_wallet) {
    engine_->LogError(FROM_HERE) << "Rewards wallet is null";
    return std::move(callback).Run(ConnectExternalWalletResult::kUnexpected);
  }

  const std::string hashed_payment_id =
      crypto::SHA256HashString(rewards_wallet->payment_id);
  const std::string external_account_id =
      base::HexEncode(hashed_payment_id.data(), hashed_payment_id.size());

  gemini_server_.post_oauth().Request(
      external_account_id, oauth_info_.code,
      base::BindOnce(&ConnectGeminiWallet::OnAuthorize, base::Unretained(this),
                     std::move(callback)));
}

void ConnectGeminiWallet::OnAuthorize(ConnectExternalWalletCallback callback,
                                      mojom::Result result,
                                      std::string&& token) {
  if (!engine_->gemini()->GetWalletIf({mojom::WalletStatus::kNotConnected,
                                       mojom::WalletStatus::kLoggedOut})) {
    return std::move(callback).Run(ConnectExternalWalletResult::kUnexpected);
  }

  if (result != mojom::Result::OK) {
    engine_->LogError(FROM_HERE) << "Couldn't get token";
    return std::move(callback).Run(ConnectExternalWalletResult::kUnexpected);
  }

  if (token.empty()) {
    engine_->LogError(FROM_HERE) << "Token is empty";
    return std::move(callback).Run(ConnectExternalWalletResult::kUnexpected);
  }

  auto on_get_recipient_id =
      base::BindOnce(&ConnectGeminiWallet::OnGetRecipientID,
                     base::Unretained(this), std::move(callback), token);

  RequestFor<GetRecipientIDGemini>(*engine_, std::move(token))
      .Send(std::move(on_get_recipient_id));
}

void ConnectGeminiWallet::OnGetRecipientID(
    ConnectExternalWalletCallback callback,
    std::string&& token,
    endpoints::GetRecipientIDGemini::Result&& result) {
  if (!engine_->gemini()->GetWalletIf({mojom::WalletStatus::kNotConnected,
                                       mojom::WalletStatus::kLoggedOut})) {
    return std::move(callback).Run(ConnectExternalWalletResult::kUnexpected);
  }

  if (!result.has_value()) {
    return std::move(callback).Run(ConnectExternalWalletResult::kUnexpected);
  }

  auto recipient_id = std::move(result).value();
  if (recipient_id.empty()) {
    return gemini_server_.post_recipient_id().Request(
        token,
        base::BindOnce(&ConnectGeminiWallet::OnPostRecipientID,
                       base::Unretained(this), std::move(callback), token));
  }

  gemini_server_.post_account().Request(
      token, base::BindOnce(&ConnectGeminiWallet::OnPostAccount,
                            base::Unretained(this), std::move(callback), token,
                            std::move(recipient_id)));
}

void ConnectGeminiWallet::OnPostRecipientID(
    ConnectExternalWalletCallback callback,
    std::string&& token,
    mojom::Result result,
    std::string&& recipient_id) {
  if (!engine_->gemini()->GetWalletIf({mojom::WalletStatus::kNotConnected,
                                       mojom::WalletStatus::kLoggedOut})) {
    return std::move(callback).Run(ConnectExternalWalletResult::kUnexpected);
  }

  if (result == mojom::Result::EXPIRED_TOKEN) {
    engine_->LogError(FROM_HERE) << "Access token expired";
    return std::move(callback).Run(ConnectExternalWalletResult::kUnexpected);
  }

  if (result == mojom::Result::NOT_FOUND) {
    engine_->LogError(FROM_HERE) << "Unverified User";
    engine_->database()->SaveEventLog(log::kKYCRequired,
                                      constant::kWalletGemini);
    return std::move(callback).Run(ConnectExternalWalletResult::kKYCRequired);
  }

  if (result != mojom::Result::OK) {
    engine_->LogError(FROM_HERE) << "Failed to create recipient ID";
    return std::move(callback).Run(ConnectExternalWalletResult::kUnexpected);
  }

  if (recipient_id.empty()) {
    engine_->LogError(FROM_HERE) << "Recipient ID is empty";
    return std::move(callback).Run(ConnectExternalWalletResult::kUnexpected);
  }

  gemini_server_.post_account().Request(
      token, base::BindOnce(&ConnectGeminiWallet::OnPostAccount,
                            base::Unretained(this), std::move(callback), token,
                            std::move(recipient_id)));
}

void ConnectGeminiWallet::OnPostAccount(ConnectExternalWalletCallback callback,
                                        std::string&& token,
                                        std::string&& recipient_id,
                                        mojom::Result result,
                                        std::string&& linking_info,
                                        std::string&& user_name,
                                        std::string&& country_id) {
  auto wallet = engine_->gemini()->GetWalletIf(
      {mojom::WalletStatus::kNotConnected, mojom::WalletStatus::kLoggedOut});
  if (!wallet) {
    return std::move(callback).Run(ConnectExternalWalletResult::kUnexpected);
  }

  if (result == mojom::Result::EXPIRED_TOKEN) {
    engine_->LogError(FROM_HERE) << "Access token expired";
    return std::move(callback).Run(ConnectExternalWalletResult::kUnexpected);
  }

  if (result != mojom::Result::OK) {
    engine_->LogError(FROM_HERE) << "Failed to get account info";
    return std::move(callback).Run(ConnectExternalWalletResult::kUnexpected);
  }

  wallet->user_name = std::move(user_name);
  if (!engine_->gemini()->SetWallet(std::move(wallet))) {
    engine_->LogError(FROM_HERE)
        << "Failed to save " << constant::kWalletGemini << " wallet";
    return std::move(callback).Run(ConnectExternalWalletResult::kUnexpected);
  }

  auto on_connect =
      base::BindOnce(&ConnectGeminiWallet::OnConnect, base::Unretained(this),
                     std::move(callback), std::move(token), recipient_id);

  RequestFor<PostConnectGemini>(*engine_, std::move(linking_info),
                                std::move(recipient_id))
      .Send(std::move(on_connect));
}

}  // namespace gemini

}  // namespace brave_rewards::internal
