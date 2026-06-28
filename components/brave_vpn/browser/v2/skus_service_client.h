/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_SKUS_SERVICE_CLIENT_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_SKUS_SERVICE_CLIENT_H_

#include <string>

#include "base/functional/callback.h"
#include "brave/components/skus/common/skus_sdk.mojom.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/remote.h"

namespace brave_vpn::v2 {

using GetSkusServiceCallback =
    base::RepeatingCallback<mojo::PendingRemote<skus::mojom::SkusService>()>;

// SkusServiceClient owns the connection to the SKUS mojom service and hides its
// lifecycle from BraveVpnService. Callers ask for a credential summary or a
// presentation; this class lazily binds the remote, re-binds it on disconnect,
// and forwards the call.
class SkusServiceClient {
 public:
  explicit SkusServiceClient(GetSkusServiceCallback getter);
  virtual ~SkusServiceClient();

  SkusServiceClient(const SkusServiceClient&) = delete;
  SkusServiceClient& operator=(const SkusServiceClient&) = delete;

  virtual void GetCredentialSummary(
      const std::string& domain,
      skus::mojom::SkusService::CredentialSummaryCallback callback);

  virtual void PrepareCredentialsPresentation(
      const std::string& domain,
      const std::string& path,
      skus::mojom::SkusService::PrepareCredentialsPresentationCallback
          callback);

  void Reset();

 private:
  void EnsureConnected();
  void OnConnectionError();

  GetSkusServiceCallback getter_;
  mojo::Remote<skus::mojom::SkusService> service_;
};

}  // namespace brave_vpn::v2

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_SKUS_SERVICE_CLIENT_H_
