/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/webcat/ud_webcat_resolver.h"

#include <optional>
#include <utility>

#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

namespace webcat {

UdWebcatResolver::UdWebcatResolver(
    brave_wallet::JsonRpcService* json_rpc_service)
    : json_rpc_service_(json_rpc_service) {}

UdWebcatResolver::~UdWebcatResolver() = default;

void UdWebcatResolver::Resolve(const std::string& domain,
                               ResolveCallback callback) {
  if (!json_rpc_service_) {
    std::move(callback).Run(domain, std::nullopt);
    return;
  }

  json_rpc_service_->UnstoppableDomainsGetWebcatCid(
      domain,
      base::BindOnce(&UdWebcatResolver::OnGetWebcatCid,
                     base::Unretained(this), std::move(callback)));
}

void UdWebcatResolver::OnGetWebcatCid(
    ResolveCallback callback,
    const std::string& cid,
    brave_wallet::mojom::ProviderError error,
    const std::string& error_message) {
  if (error != brave_wallet::mojom::ProviderError::kSuccess || cid.empty()) {
    std::move(callback).Run(std::string(), std::nullopt);
    return;
  }

  std::move(callback).Run(std::string(), cid);
}

}  // namespace webcat
