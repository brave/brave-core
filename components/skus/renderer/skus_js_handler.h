/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SKUS_RENDERER_SKUS_JS_HANDLER_H_
#define BRAVE_COMPONENTS_SKUS_RENDERER_SKUS_JS_HANDLER_H_

#include <memory>
#include <string>
#include <vector>

#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/skus/common/skus_sdk.mojom.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_frame_observer.h"
#include "gin/wrappable.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "url/gurl.h"
#include "v8/include/v8.h"

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/components/brave_vpn/common/mojom/brave_vpn.mojom.h"
#endif

namespace skus {

// If present, this will inject a few methods (used by SKU SDK)
// into window.chrome.braveSkus.*
//
// This is only intended to be used on account.brave.com and the dev / staging
// counterparts. The accounts website will use this if present which allows a
// safe way for the browser to intercept credentials which are used in the
// browser.
//
// The first use-case for this credential redemption is with VPN. Folks
// will be able to purchase VPN from account.brave.com and the browser can
// detect the purchase and use those credentials during authentication when
// establishing a connection to our partner providing the VPN service.
class SkusJSHandler : public content::RenderFrameObserver,
                      public gin::Wrappable<SkusJSHandler> {
 public:
  explicit SkusJSHandler(content::RenderFrame* render_frame);
  SkusJSHandler(const SkusJSHandler&) = delete;
  SkusJSHandler& operator=(const SkusJSHandler&) = delete;
  ~SkusJSHandler() override;

  static gin::WrapperInfo kWrapperInfo;

  static void Install(content::RenderFrame* render_frame);

  // content::RenderFrameObserver:
  void OnDestruct() override;

 private:
  bool EnsureConnected();

  // gin::WrappableBase
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;

  // window.chrome.braveSkus.refresh_order
  v8::Local<v8::Promise> RefreshOrder(v8::Isolate* isolate,
                                      std::string order_id);
  void OnRefreshOrder(v8::Global<v8::Promise::Resolver> promise_resolver,
                      v8::Isolate* isolate,
                      v8::Global<v8::Context> context_old,
                      skus::mojom::SkusResultPtr response);

  // window.chrome.braveSkus.fetch_order_credentials
  v8::Local<v8::Promise> FetchOrderCredentials(v8::Isolate* isolate,
                                               std::string order_id);
  void OnFetchOrderCredentials(
      v8::Global<v8::Promise::Resolver> promise_resolver,
      v8::Isolate* isolate,
      v8::Global<v8::Context> context_old,
      skus::mojom::SkusResultPtr response);

  // window.chrome.braveSkus.prepare_credentials_presentation
  v8::Local<v8::Promise> PrepareCredentialsPresentation(v8::Isolate* isolate,
                                                        std::string domain,
                                                        std::string path);
  void OnPrepareCredentialsPresentation(
      v8::Global<v8::Promise::Resolver> promise_resolver,
      v8::Isolate* isolate,
      v8::Global<v8::Context> context_old,
      skus::mojom::SkusResultPtr response);

  // window.chrome.braveSkus.credential_summary
  v8::Local<v8::Promise> CredentialSummary(v8::Isolate* isolate,
                                           std::string domain);
  void OnCredentialSummary(const std::string& domain,
                           v8::Global<v8::Promise::Resolver> promise_resolver,
                           v8::Isolate* isolate,
                           v8::Global<v8::Context> context_old,
                           skus::mojom::SkusResultPtr response);

  mojo::Remote<skus::mojom::SkusService> skus_service_;
#if BUILDFLAG(ENABLE_BRAVE_VPN)
  mojo::Remote<brave_vpn::mojom::ServiceHandler> vpn_service_;
#endif
};

}  // namespace skus

#endif  // BRAVE_COMPONENTS_SKUS_RENDERER_SKUS_JS_HANDLER_H_
