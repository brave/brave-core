/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/solana_provider_impl.h"

#include <utility>

#include "base/notreached.h"

namespace brave_wallet {

SolanaProviderImpl::SolanaProviderImpl() = default;
SolanaProviderImpl::~SolanaProviderImpl() = default;

void SolanaProviderImpl::Connect(ConnectCallback callback) {
  // NOTIMPLEMENTED();
  // std::move(callback).Run(mojom::SolanaProviderError::kInternalError, "",
  // "");
  std::move(callback).Run(mojom::SolanaProviderError::kSuccess, "",
                          "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8");
}

void SolanaProviderImpl::Disconnect() {
  NOTIMPLEMENTED();
}

}  // namespace brave_wallet
