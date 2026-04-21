/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/webcat/ens_webcat_resolver.h"

#include <optional>
#include <utility>

#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/webcat/core/constants.h"

namespace webcat {

EnsWebcatResolver::EnsWebcatResolver(
    brave_wallet::JsonRpcService* json_rpc_service)
    : json_rpc_service_(json_rpc_service) {}

EnsWebcatResolver::~EnsWebcatResolver() = default;

void EnsWebcatResolver::Resolve(const std::string& domain,
                                ResolveCallback callback) {
  if (!json_rpc_service_) {
    std::move(callback).Run(domain, std::nullopt);
    return;
  }

  json_rpc_service_->EnsGetTextRecord(
      domain, std::string(kWebcatEnsTextRecordKey),
      base::BindOnce(&EnsWebcatResolver::OnEnsGetTextRecord,
                     base::Unretained(this), std::move(callback)));
}

void EnsWebcatResolver::OnEnsGetTextRecord(
    ResolveCallback callback,
    const std::string& value,
    bool require_offchain_consent,
    brave_wallet::mojom::ProviderError error,
    const std::string& error_message) {
  if (error != brave_wallet::mojom::ProviderError::kSuccess || value.empty()) {
    std::move(callback).Run(std::string(), std::nullopt);
    return;
  }

  std::move(callback).Run(std::string(), value);
}

}  // namespace webcat
