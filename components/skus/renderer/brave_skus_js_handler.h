/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SKUS_RENDERER_BRAVE_SKUS_JS_HANDLER_H_
#define BRAVE_COMPONENTS_SKUS_RENDERER_BRAVE_SKUS_JS_HANDLER_H_

#include <memory>
#include <string>
#include <vector>

#include "brave/components/skus/common/skus_sdk.mojom.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_frame_observer.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "url/gurl.h"
#include "v8/include/v8.h"

namespace brave_rewards {

// If present, this will inject a few methods (used by SKU SDK)
// into window.brave.*
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
class BraveSkusJSHandler {
 public:
  explicit BraveSkusJSHandler(content::RenderFrame* render_frame);
  BraveSkusJSHandler(const BraveSkusJSHandler&) = delete;
  BraveSkusJSHandler& operator=(const BraveSkusJSHandler&) = delete;
  ~BraveSkusJSHandler();

  void AddJavaScriptObjectToFrame(v8::Local<v8::Context> context);
  void ResetRemote(content::RenderFrame* render_frame);

 private:
  template <typename Sig>
  void BindFunctionToObject(v8::Isolate* isolate,
                            v8::Local<v8::Object> javascript_object,
                            const std::string& name,
                            const base::RepeatingCallback<Sig>& callback);
  void BindFunctionsToObject(v8::Isolate* isolate,
                             v8::Local<v8::Context> context);
  bool EnsureConnected();

  // window.brave.skus.refresh_order
  v8::Local<v8::Promise> RefreshOrder(v8::Isolate* isolate,
                                      std::string order_id);
  void OnRefreshOrder(v8::Global<v8::Promise::Resolver> promise_resolver,
                      v8::Isolate* isolate,
                      v8::Global<v8::Context> context_old,
                      const std::string& response);

  // window.brave.skus.fetch_order_credentials
  v8::Local<v8::Promise> FetchOrderCredentials(v8::Isolate* isolate,
                                               std::string order_id);
  void OnFetchOrderCredentials(
      v8::Global<v8::Promise::Resolver> promise_resolver,
      v8::Isolate* isolate,
      v8::Global<v8::Context> context_old,
      const std::string& response);

  // window.brave.skus.prepare_credentials_presentation
  v8::Local<v8::Promise> PrepareCredentialsPresentation(v8::Isolate* isolate,
                                                        std::string domain,
                                                        std::string path);
  void OnPrepareCredentialsPresentation(
      v8::Global<v8::Promise::Resolver> promise_resolver,
      v8::Isolate* isolate,
      v8::Global<v8::Context> context_old,
      const std::string& response);

  // window.brave.skus.credential_summary
  v8::Local<v8::Promise> CredentialSummary(v8::Isolate* isolate,
                                           std::string domain);
  void OnCredentialSummary(v8::Global<v8::Promise::Resolver> promise_resolver,
                           v8::Isolate* isolate,
                           v8::Global<v8::Context> context_old,
                           const std::string& response);

  content::RenderFrame* render_frame_;
  mojo::Remote<skus::mojom::SkusSdk> skus_sdk_;
};

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_SKUS_RENDERER_BRAVE_SKUS_JS_HANDLER_H_
