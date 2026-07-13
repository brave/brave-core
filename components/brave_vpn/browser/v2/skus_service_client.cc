/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/v2/skus_service_client.h"

#include <string>
#include <utility>

#include "base/check.h"
#include "base/functional/callback.h"
#include "base/logging.h"
#include "base/task/bind_post_task.h"
#include "brave/components/skus/common/skus_sdk.mojom.h"
#include "mojo/public/cpp/bindings/callback_helpers.h"

namespace brave_vpn::v2 {
namespace {
// Guarantees the "runs exactly once" callback contract: Mojo destroys pending
// response callbacks without invoking them when the pipe disconnects or the
// Remote is reset, which would otherwise leave the caller waiting forever. The
// wrapper turns a dropped callback into a synthesized error response. The
// default-invoke fires synchronously during pipe teardown, so the caller's
// callback is additionally bound through the current task runner to deliver the
// response asynchronously.
base::OnceCallback<void(skus::mojom::SkusResultPtr)> WrapResultCallback(
    base::OnceCallback<void(skus::mojom::SkusResultPtr)> callback) {
  return mojo::WrapCallbackWithDefaultInvokeIfNotRun(
      base::BindPostTaskToCurrentDefault(std::move(callback)),
      skus::mojom::SkusResult::New(skus::mojom::SkusResultCode::UnknownError,
                                   "SkusService disconnected"));
}
}  // namespace

SkusServiceClient::SkusServiceClient(GetSkusServiceCallback getter)
    : getter_(std::move(getter)) {
  CHECK(getter_);
}

SkusServiceClient::~SkusServiceClient() = default;

void SkusServiceClient::GetCredentialSummary(
    const std::string& domain,
    skus::mojom::SkusService::CredentialSummaryCallback callback) {
  EnsureConnected();
  service_->CredentialSummary(domain, WrapResultCallback(std::move(callback)));
}

void SkusServiceClient::PrepareCredentialsPresentation(
    const std::string& domain,
    const std::string& path,
    skus::mojom::SkusService::PrepareCredentialsPresentationCallback callback) {
  EnsureConnected();
  service_->PrepareCredentialsPresentation(
      domain, path, WrapResultCallback(std::move(callback)));
}

void SkusServiceClient::Reset() {
  // Dropping the remote destroys any pending response callbacks; the
  // WrapResultCallback wrapper turns each into a posted error response.
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
  service_.reset_on_disconnect();
}

}  // namespace brave_vpn::v2
