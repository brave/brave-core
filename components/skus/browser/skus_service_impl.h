// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_SKUS_BROWSER_SKUS_SERVICE_IMPL_H_
#define BRAVE_COMPONENTS_SKUS_BROWSER_SKUS_SERVICE_IMPL_H_

#include <memory>
#include <string>

#include "base/memory/weak_ptr.h"
#include "brave/components/skus/browser/rs/cxx/src/shim.h"
#include "brave/components/skus/common/skus_sdk.mojom.h"
#include "components/keyed_service/core/keyed_service.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote_set.h"

class PrefService;

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace skus {

struct CppSDK;
class SkusContextImpl;

// This is only intended to be used on account.brave.com and the dev / staging
// counterparts. The accounts website will use this if present which allows a
// safe way for the browser to intercept credentials which are used in the
// browser.
//
// The first use-case for this credential redemption is with VPN. Folks
// will be able to purchase VPN from account.brave.com and the browser can
// detect the purchase and use those credentials during authentication when
// establishing a connection to our partner providing the VPN service.
//
// There are a few different implementations using this service:
// 1. RenderFrameObserver will (conditionally) inject a handler which uses
//    Mojom to provide to call this in the browser process. See
//    `components/skus/renderer/skus_js_handler.h` for more info.
//
// 2. The service can be called directly. For example, if we intercept the
//    order / credential process for a person purchasing VPN, we may only
//    call `CredentialSummary` to verify a credential exists (this never
//    exposes the credentials). When the VPN service itself NEEDS the
//    credentials, it can use this service to call
//    `PrepareCredentialsPresentation`. If the credentials expire, the VPN
//    service can call `FetchOrderCredentials`
//
// 3. iOS will need to have a JS injection where the native handler can call
//    this service. See https://github.com/brave/brave-ios/issues/4804
//
class SkusServiceImpl : public KeyedService, public mojom::SkusService {
 public:
  explicit SkusServiceImpl(
      PrefService* prefs,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~SkusServiceImpl() override;

  SkusServiceImpl(const SkusServiceImpl&) = delete;
  SkusServiceImpl& operator=(SkusServiceImpl&) = delete;

  mojo::PendingRemote<mojom::SkusService> MakeRemote();
  void Bind(mojo::PendingReceiver<mojom::SkusService> receiver);

  // KeyedService
  void Shutdown() override;

  // mojom::SkusService
  void RefreshOrder(
      const std::string& order_id,
      skus::mojom::SkusService::RefreshOrderCallback callback) override;
  void FetchOrderCredentials(
      const std::string& order_id,
      skus::mojom::SkusService::FetchOrderCredentialsCallback callback)
      override;
  void PrepareCredentialsPresentation(
      const std::string& domain,
      const std::string& path,
      skus::mojom::SkusService::PrepareCredentialsPresentationCallback callback)
      override;
  void CredentialSummary(
      const std::string& domain,
      skus::mojom::SkusService::CredentialSummaryCallback callback) override;

 private:
  void OnCredentialSummary(
      const std::string& domain,
      mojom::SkusService::CredentialSummaryCallback callback,
      const std::string& summary_string);

  std::unique_ptr<skus::SkusContextImpl> context_;
  ::rust::Box<skus::CppSDK> sdk_;
  mojo::ReceiverSet<mojom::SkusService> receivers_;
  base::WeakPtrFactory<SkusServiceImpl> weak_factory_{this};
};

}  // namespace skus

#endif  // BRAVE_COMPONENTS_SKUS_BROWSER_SKUS_SERVICE_IMPL_H_
