/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/wallet_provider/gemini/connect_gemini_wallet.h"

#include <utility>

#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_rewards/core/common/random_util.h"
#include "brave/components/brave_rewards/core/database/database.h"
#include "brave/components/brave_rewards/core/endpoint/gemini/gemini_server.h"
#include "brave/components/brave_rewards/core/endpoints/brave/post_connect_gemini.h"
#include "brave/components/brave_rewards/core/endpoints/request_for.h"
#include "brave/components/brave_rewards/core/gemini/gemini.h"
#include "brave/components/brave_rewards/core/gemini/gemini_util.h"
#include "brave/components/brave_rewards/core/global_constants.h"
#include "brave/components/brave_rewards/core/logging/event_log_keys.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"
#include "brave/components/brave_rewards/core/wallet/wallet.h"
#include "crypto/sha2.h"

namespace brave_rewards::internal {

using endpoints::GetRecipientIDGemini;
using endpoints::PostConnectGemini;
using endpoints::RequestFor;
using wallet_provider::ConnectExternalWallet;

namespace gemini {

ConnectGeminiWallet::ConnectGeminiWallet(RewardsEngineImpl& engine)
    : ConnectExternalWallet(engine), gemini_server_(engine) {}

ConnectGeminiWallet::~ConnectGeminiWallet() = default;

const char* ConnectGeminiWallet::WalletType() const {
  return constant::kWalletGemini;
}

void ConnectGeminiWallet::Authorize(OAuthInfo&& oauth_info,
                                    ConnectExternalWalletCallback callback) {
  DCHECK(!oauth_info.code.empty());

  const auto rewards_wallet = engine_->wallet()->GetWallet();
  if (!rewards_wallet) {
    BLOG(0, "Rewards wallet is null!");
    return std::move(callback).Run(
        base::unexpected(mojom::ConnectExternalWalletError::kUnexpected));
  }

  const std::string hashed_payment_id =
      crypto::SHA256HashString(rewards_wallet->payment_id);
  const std::string external_account_id =
      base::HexEncode(hashed_payment_id.data(), hashed_payment_id.size());

  gemini_server_.post_oauth().Request(
      external_account_id, std::move(oauth_info.code),
      base::BindOnce(&ConnectGeminiWallet::OnAuthorize, base::Unretained(this),
                     std::move(callback)));
}

void ConnectGeminiWallet::OnAuthorize(ConnectExternalWalletCallback callback,
                                      mojom::Result result,
                                      std::string&& token) {
  if (!engine_->gemini()->GetWalletIf({mojom::WalletStatus::kNotConnected,
                                       mojom::WalletStatus::kLoggedOut})) {
    return std::move(callback).Run(
        base::unexpected(mojom::ConnectExternalWalletError::kUnexpected));
  }

  if (result != mojom::Result::OK) {
    BLOG(0, "Couldn't get token");
    return std::move(callback).Run(
        base::unexpected(mojom::ConnectExternalWalletError::kUnexpected));
  }

  if (token.empty()) {
    BLOG(0, "Token is empty");
    return std::move(callback).Run(
        base::unexpected(mojom::ConnectExternalWalletError::kUnexpected));
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
    return std::move(callback).Run(
        base::unexpected(mojom::ConnectExternalWalletError::kUnexpected));
  }

  if (!result.has_value()) {
    return std::move(callback).Run(
        base::unexpected(mojom::ConnectExternalWalletError::kUnexpected));
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
    return std::move(callback).Run(
        base::unexpected(mojom::ConnectExternalWalletError::kUnexpected));
  }

  if (result == mojom::Result::EXPIRED_TOKEN) {
    BLOG(0, "Access token expired!");
    return std::move(callback).Run(
        base::unexpected(mojom::ConnectExternalWalletError::kUnexpected));
  }

  if (result == mojom::Result::NOT_FOUND) {
    BLOG(0, "Unverified User");
    engine_->database()->SaveEventLog(log::kKYCRequired,
                                      constant::kWalletGemini);
    return std::move(callback).Run(
        base::unexpected(mojom::ConnectExternalWalletError::kKYCRequired));
  }

  if (result != mojom::Result::OK) {
    BLOG(0, "Failed to create recipient ID!");
    return std::move(callback).Run(
        base::unexpected(mojom::ConnectExternalWalletError::kUnexpected));
  }

  if (recipient_id.empty()) {
    BLOG(0, "Recipient ID is empty!");
    return std::move(callback).Run(
        base::unexpected(mojom::ConnectExternalWalletError::kUnexpected));
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
    return std::move(callback).Run(
        base::unexpected(mojom::ConnectExternalWalletError::kUnexpected));
  }

  if (result == mojom::Result::EXPIRED_TOKEN) {
    BLOG(0, "Access token expired!");
    return std::move(callback).Run(
        base::unexpected(mojom::ConnectExternalWalletError::kUnexpected));
  }

  if (result != mojom::Result::OK) {
    BLOG(0, "Failed to get account info!");
    return std::move(callback).Run(
        base::unexpected(mojom::ConnectExternalWalletError::kUnexpected));
  }

  wallet->user_name = std::move(user_name);
  if (!engine_->gemini()->SetWallet(std::move(wallet))) {
    BLOG(0, "Failed to save " << constant::kWalletGemini << " wallet!");
    return std::move(callback).Run(
        base::unexpected(mojom::ConnectExternalWalletError::kUnexpected));
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
