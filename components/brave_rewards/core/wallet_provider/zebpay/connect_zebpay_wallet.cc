/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/wallet_provider/zebpay/connect_zebpay_wallet.h"

#include <string>
#include <utility>

#include "brave/components/brave_rewards/core/endpoints/brave/post_connect_zebpay.h"
#include "brave/components/brave_rewards/core/endpoints/request_for.h"
#include "brave/components/brave_rewards/core/global_constants.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"
#include "brave/components/brave_rewards/core/zebpay/zebpay.h"
#include "brave/components/brave_rewards/core/zebpay/zebpay_util.h"

using brave_rewards::internal::endpoints::PostConnectZebPay;
using brave_rewards::internal::endpoints::PostOAuthZebPay;
using brave_rewards::internal::endpoints::RequestFor;
using brave_rewards::internal::wallet_provider::ConnectExternalWallet;
using brave_rewards::mojom::ConnectExternalWalletResult;

namespace brave_rewards::internal::zebpay {

ConnectZebPayWallet::ConnectZebPayWallet(RewardsEngineImpl& engine)
    : ConnectExternalWallet(engine) {}

ConnectZebPayWallet::~ConnectZebPayWallet() = default;

const char* ConnectZebPayWallet::WalletType() const {
  return constant::kWalletZebPay;
}

std::string ConnectZebPayWallet::GetOAuthLoginURL() const {
  return GetLoginUrl(oauth_info_.one_time_string);
}

void ConnectZebPayWallet::Authorize(ConnectExternalWalletCallback callback) {
  DCHECK(!oauth_info_.code.empty());

  RequestFor<PostOAuthZebPay>(*engine_, oauth_info_.code)
      .Send(base::BindOnce(&ConnectZebPayWallet::OnAuthorize,
                           base::Unretained(this), std::move(callback)));
}

void ConnectZebPayWallet::OnAuthorize(
    ConnectExternalWalletCallback callback,
    endpoints::PostOAuthZebPay::Result&& result) const {
  if (!engine_->zebpay()->GetWalletIf({mojom::WalletStatus::kNotConnected,
                                       mojom::WalletStatus::kLoggedOut})) {
    return std::move(callback).Run(ConnectExternalWalletResult::kUnexpected);
  }

  if (!result.has_value()) {
    BLOG(0, "Couldn't exchange code for the access token!");
    return std::move(callback).Run(ConnectExternalWalletResult::kUnexpected);
  }

  auto [access_token, linking_info, deposit_id] = std::move(result.value());

  auto on_connect = base::BindOnce(
      &ConnectZebPayWallet::OnConnect, base::Unretained(this),
      std::move(callback), std::move(access_token), std::move(deposit_id));

  RequestFor<PostConnectZebPay>(*engine_, std::move(linking_info))
      .Send(std::move(on_connect));
}

}  // namespace brave_rewards::internal::zebpay
