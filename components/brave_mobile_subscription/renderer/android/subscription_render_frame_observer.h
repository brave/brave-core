// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_MOBILE_SUBSCRIPTION_RENDERER_ANDROID_SUBSCRIPTION_RENDER_FRAME_OBSERVER_H_
#define BRAVE_COMPONENTS_BRAVE_MOBILE_SUBSCRIPTION_RENDERER_ANDROID_SUBSCRIPTION_RENDER_FRAME_OBSERVER_H_

#include <memory>
#include <optional>
#include <string>

#include "base/gtest_prod_util.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_frame_observer.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "v8/include/v8.h"

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/components/brave_vpn/common/mojom/brave_vpn.mojom.h"
#endif

namespace brave_subscription {

// Used on Android to conditionally inject the purchase token (via local
// storage) for Brave VPN purchased on the Google Play Store. The Brave accounts
// website will use this to link the purchase to a desktop credential.
//
// Implementation-wise, those methods will only resolve in a regular
// (non-private / non-guest / non-Tor) context.
//
// See `renderer/brave_content_renderer_client.cc` for more information.
class SubscriptionRenderFrameObserver : public content::RenderFrameObserver {
 public:
  explicit SubscriptionRenderFrameObserver(content::RenderFrame* render_frame,
                                           int32_t world_id);
  SubscriptionRenderFrameObserver(const SubscriptionRenderFrameObserver&) =
      delete;
  SubscriptionRenderFrameObserver& operator=(
      const SubscriptionRenderFrameObserver&) = delete;
  ~SubscriptionRenderFrameObserver() override;

  // RenderFrameObserver implementation.
  void DidCreateScriptContext(v8::Local<v8::Context> context,
                              int32_t world_id) override;

 private:
  FRIEND_TEST_ALL_PREFIXES(SubscriptionRenderFrameObserverBrowserTest,
                           IsAllowed);
  FRIEND_TEST_ALL_PREFIXES(SubscriptionRenderFrameObserverTest, ExtractParam);
  FRIEND_TEST_ALL_PREFIXES(SubscriptionRenderFrameObserverTest, IsValueAllowed);
  enum class Product { kVPN = 0, kLeo = 1 };

  bool EnsureConnected();
  void OnGetPurchaseToken(const std::string& purchase_token);
  void OnGetPurchaseTokenOrderId(const std::string& purchase_token,
                                 const std::string& order_id);
  std::string ExtractParam(const GURL& url, const std::string& name) const;
  bool IsValueAllowed(const std::string& purchase_token) const;

  // RenderFrameObserver implementation.
  void OnDestruct() override;

  bool IsAllowed();

  std::string GetPurchaseTokenJSString(const std::string& purchase_token);

  const int32_t world_id_;
  std::optional<Product> product_ = std::nullopt;
#if BUILDFLAG(ENABLE_BRAVE_VPN)
  mojo::Remote<brave_vpn::mojom::ServiceHandler> vpn_service_;
#endif
  mojo::Remote<ai_chat::mojom::IAPSubscription> ai_chat_subscription_;
  base::WeakPtrFactory<SubscriptionRenderFrameObserver> weak_factory_{this};
};

}  // namespace brave_subscription

#endif  // BRAVE_COMPONENTS_BRAVE_MOBILE_SUBSCRIPTION_RENDERER_ANDROID_SUBSCRIPTION_RENDER_FRAME_OBSERVER_H_
