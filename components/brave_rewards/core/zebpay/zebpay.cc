/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <utility>

#include "brave/components/brave_rewards/core/endpoints/request_for.h"
#include "brave/components/brave_rewards/core/global_constants.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"
#include "brave/components/brave_rewards/core/wallet/wallet_util.h"
#include "brave/components/brave_rewards/core/zebpay/zebpay.h"

using brave_rewards::internal::endpoints::GetBalanceZebPay;
using brave_rewards::internal::endpoints::RequestFor;

namespace brave_rewards::internal::zebpay {

ZebPay::ZebPay(RewardsEngineImpl& engine)
    : engine_(engine), connect_wallet_(engine), get_wallet_(engine) {}

ZebPay::~ZebPay() = default;

void ZebPay::FetchBalance(FetchBalanceCallback callback) {
  auto wallet = GetWalletIf({mojom::WalletStatus::kConnected});
  if (!wallet) {
    return std::move(callback).Run(mojom::Result::FAILED, 0.0);
  }

  RequestFor<GetBalanceZebPay>(*engine_, std::move(wallet->token))
      .Send(base::BindOnce(&ZebPay::OnFetchBalance, base::Unretained(this),
                           std::move(callback)));
}

void ZebPay::OnFetchBalance(FetchBalanceCallback callback,
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

        BLOG(0,
             "Failed to disconnect " << constant::kWalletZebPay << " wallet!");
        ABSL_FALLTHROUGH_INTENDED;
      default:
        return std::move(callback).Run(mojom::Result::FAILED, 0.0);
    }
  }

  std::move(callback).Run(mojom::Result::OK, result.value());
}

void ZebPay::ConnectWallet(const base::flat_map<std::string, std::string>& args,
                           ConnectExternalWalletCallback callback) {
  connect_wallet_.Run(args, std::move(callback));
}

void ZebPay::GetWallet(GetExternalWalletCallback callback) {
  get_wallet_.Run(std::move(callback));
}

mojom::ExternalWalletPtr ZebPay::GetWallet() {
  return wallet::GetWallet(*engine_, constant::kWalletZebPay);
}

mojom::ExternalWalletPtr ZebPay::GetWalletIf(
    const std::set<mojom::WalletStatus>& statuses) {
  return wallet::GetWalletIf(*engine_, constant::kWalletZebPay, statuses);
}

bool ZebPay::SetWallet(mojom::ExternalWalletPtr wallet) {
  return wallet::SetWallet(*engine_, std::move(wallet));
}

bool ZebPay::LogOutWallet() {
  return wallet::LogOutWallet(*engine_, constant::kWalletZebPay);
}

}  // namespace brave_rewards::internal::zebpay
