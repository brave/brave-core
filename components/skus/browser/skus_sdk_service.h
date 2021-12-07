// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_SKUS_BROWSER_SKUS_SDK_SERVICE_H_
#define BRAVE_COMPONENTS_SKUS_BROWSER_SKUS_SDK_SERVICE_H_

#include <memory>
#include <string>

#include "base/memory/weak_ptr.h"
#include "brave/components/skus/browser/skus_sdk_context_impl.h"
#include "brave/components/skus/common/skus_sdk.mojom.h"
#include "brave/third_party/rust/cxx/include/cxx.h"
#include "components/keyed_service/core/keyed_service.h"

class PrefService;

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace skus {

struct CppSDK;

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
//    `brave_skus_js_handler.h` for more info.
//
// 2. The service can be called directly. For example, if we intercept the
//    order / credential process for a person purchasing VPN, we may only
//    call `CredentialSummary` to verify a credential exists (this never
//    exposes the credentials). When the VPN service itself NEEDS the
//    credentials, it can use this service to call
//    `PrepareCredentialsPresentation`. If the credentials expire, the VPN
//    service can call `FetchOrderCredentials`
//
// This implementation is meant to work on Android, Desktop, and iOS.
// iOS will need to have a JS injection where the native handler can call
// this service.
//
// For more information, please see:
// https://github.com/brave-intl/br-rs/tree/skus
class SkusSdkService : public KeyedService, public skus::mojom::SkusSdk {
 public:
  explicit SkusSdkService(
      PrefService* prefs,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~SkusSdkService() override;

  SkusSdkService(const SkusSdkService&) = delete;
  SkusSdkService& operator=(SkusSdkService&) = delete;

  void RefreshOrder(
      const std::string& order_id,
      skus::mojom::SkusSdk::RefreshOrderCallback callback) override;
  void FetchOrderCredentials(
      const std::string& order_id,
      skus::mojom::SkusSdk::FetchOrderCredentialsCallback callback) override;
  void PrepareCredentialsPresentation(
      const std::string& domain,
      const std::string& path,
      skus::mojom::SkusSdk::PrepareCredentialsPresentationCallback callback)
      override;
  void CredentialSummary(
      const std::string& domain,
      skus::mojom::SkusSdk::CredentialSummaryCallback callback) override;

 private:
  std::unique_ptr<skus::SkusSdkContextImpl> context_;
  ::rust::Box<skus::CppSDK> sdk_;
  PrefService* prefs_;
  base::WeakPtrFactory<SkusSdkService> weak_factory_;
};

}  // namespace skus

#endif  // BRAVE_COMPONENTS_SKUS_BROWSER_SKUS_SDK_SERVICE_H_
