/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/services/brave_wallet/brave_wallet_utils_service_impl.h"

#include <memory>
#include <utility>

#include "brave/components/services/brave_wallet/zcash/zcash_decoder.h"

namespace brave_wallet {

BraveWalletUtilsServiceImpl::BraveWalletUtilsServiceImpl(
    mojo::PendingReceiver<mojom::BraveWalletUtilsService> receiver)
    : receiver_(this, std::move(receiver)) {}

BraveWalletUtilsServiceImpl::~BraveWalletUtilsServiceImpl() = default;

void BraveWalletUtilsServiceImpl::CreateZCashDecoderService(
    mojo::PendingAssociatedReceiver<zcash::mojom::ZCashDecoder>
        zcash_decoder_receiver) {
  if (!instance_) {
    instance_ = mojo::MakeSelfOwnedAssociatedReceiver(
        std::make_unique<ZCashDecoder>(), std::move(zcash_decoder_receiver));
  }
}

}  // namespace brave_wallet
