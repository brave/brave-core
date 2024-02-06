/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/zebpay/zebpay.h"

#include <memory>
#include <utility>

#include "brave/components/brave_rewards/common/mojom/rewards.mojom.h"
#include "brave/components/brave_rewards/core/common/environment_config.h"
#include "brave/components/brave_rewards/core/endpoints/request_for.h"
#include "brave/components/brave_rewards/core/global_constants.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"
#include "brave/components/brave_rewards/core/wallet_provider/zebpay/connect_zebpay_wallet.h"

using brave_rewards::internal::endpoints::GetBalanceZebPay;
using brave_rewards::internal::endpoints::RequestFor;

namespace brave_rewards::internal::zebpay {

ZebPay::ZebPay(RewardsEngineImpl& engine)
    : WalletProvider(engine), engine_(engine) {
  connect_wallet_ = std::make_unique<ConnectZebPayWallet>(engine);
}

const char* ZebPay::WalletType() const {
  return constant::kWalletZebPay;
}

void ZebPay::AssignWalletLinks(mojom::ExternalWallet& external_wallet) {
  auto url = engine_->Get<EnvironmentConfig>().zebpay_api_url();
  external_wallet.account_url = url.Resolve("/dashboard").spec();
  external_wallet.activity_url = url.Resolve("/activity").spec();
}

void ZebPay::FetchBalance(
    base::OnceCallback<void(mojom::Result, double)> callback) {
  auto wallet = GetWalletIf({mojom::WalletStatus::kConnected});
  if (!wallet) {
    return std::move(callback).Run(mojom::Result::FAILED, 0.0);
  }

  RequestFor<GetBalanceZebPay>(*engine_, std::move(wallet->token))
      .Send(base::BindOnce(&ZebPay::OnFetchBalance, base::Unretained(this),
                           std::move(callback)));
}

void ZebPay::OnFetchBalance(
    base::OnceCallback<void(mojom::Result, double)> callback,
    GetBalanceZebPay::Result&& result) {
  if (!GetWalletIf({mojom::WalletStatus::kConnected})) {
    return std::move(callback).Run(mojom::Result::FAILED, 0.0);
  }

  if (!result.has_value()) {
    switch (result.error()) {
      case GetBalanceZebPay::Error::kAccessTokenExpired:
        if (LogOutWallet()) {
          return std::move(callback).Run(mojom::Result::EXPIRED_TOKEN, 0.0);
        }

        engine_->LogError(FROM_HERE) << "Failed to disconnect zebpay wallet";
        ABSL_FALLTHROUGH_INTENDED;
      default:
        return std::move(callback).Run(mojom::Result::FAILED, 0.0);
    }
  }

  std::move(callback).Run(mojom::Result::OK, result.value());
}

std::string ZebPay::GetFeeAddress() const {
  return "";
}

}  // namespace brave_rewards::internal::zebpay
