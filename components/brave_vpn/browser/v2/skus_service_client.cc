/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/v2/skus_service_client.h"

#include <string>
#include <utility>

#include "base/check.h"
#include "base/logging.h"
#include "brave/components/skus/common/skus_sdk.mojom.h"

namespace brave_vpn::v2 {

SkusServiceClient::SkusServiceClient(GetSkusServiceCallback getter)
    : getter_(std::move(getter)) {
  DCHECK(getter_);
}

SkusServiceClient::~SkusServiceClient() = default;

void SkusServiceClient::GetCredentialSummary(
    const std::string& domain,
    skus::mojom::SkusService::CredentialSummaryCallback callback) {
  EnsureConnected();
  service_->CredentialSummary(domain, std::move(callback));
}

void SkusServiceClient::PrepareCredentialsPresentation(
    const std::string& domain,
    const std::string& path,
    skus::mojom::SkusService::PrepareCredentialsPresentationCallback callback) {
  EnsureConnected();
  service_->PrepareCredentialsPresentation(domain, path, std::move(callback));
}

void SkusServiceClient::Reset() {
  service_.reset();
}

void SkusServiceClient::EnsureConnected() {
  if (service_) {
    return;
  }
  VLOG(2) << "Connecting to SkusService";
  auto pending = getter_.Run();
  service_.Bind(std::move(pending));
  DCHECK(service_);
  service_.set_disconnect_handler(base::BindOnce(
      &SkusServiceClient::OnConnectionError, base::Unretained(this)));
}

void SkusServiceClient::OnConnectionError() {
  VLOG(2) << "Reconnecting to SkusService";
  service_.reset();
  EnsureConnected();
}

}  // namespace brave_vpn::v2
