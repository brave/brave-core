/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/services/brave_wallet/public/cpp/brave_wallet_utils_service.h"

#include <utility>

#include "base/no_destructor.h"
#include "build/build_config.h"

#if BUILDFLAG(IS_IOS)
#include "brave/components/services/brave_wallet/public/cpp/brave_wallet_utils_service_in_process_launcher.h"
#else
#include "brave/components/services/brave_wallet/content/brave_wallet_utils_service_launcher.h"
#endif

namespace brave_wallet {

BraveWalletUtilsService::BraveWalletUtilsService() = default;
BraveWalletUtilsService::~BraveWalletUtilsService() = default;

// static
BraveWalletUtilsService* BraveWalletUtilsService::GetInstance() {
  static base::NoDestructor<BraveWalletUtilsService> service;
  return service.get();
}

void BraveWalletUtilsService::CreateZCashDecoder(
    mojo::PendingAssociatedReceiver<zcash::mojom::ZCashDecoder> receiver) {
  MaybeLaunchService();
  brave_wallet_utils_service_->CreateZCashDecoderService(std::move(receiver));
}

void BraveWalletUtilsService::MaybeLaunchService() {
#if BUILDFLAG(IS_IOS)
  LaunchInProcessBraveWalletUtilsService(
      brave_wallet_utils_service_.BindNewPipeAndPassReceiver());
  brave_wallet_utils_service_.reset_on_disconnect();
#else
  LaunchBraveWalletUtilsService(
      brave_wallet_utils_service_.BindNewPipeAndPassReceiver());
  brave_wallet_utils_service_.reset_on_disconnect();
  // 10 minutes is a default wallet lock time
  brave_wallet_utils_service_.reset_on_idle_timeout(base::Minutes(10));
#endif
}

}  // namespace brave_wallet
