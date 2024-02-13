/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ANDROID_AI_CHAT_IAP_SUBSCRIPTION_ANDROID_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ANDROID_AI_CHAT_IAP_SUBSCRIPTION_ANDROID_H_

#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"

class PrefService;

namespace ai_chat {

// AIChatIAPSubscription is a class that is responsible for interaction
// between SubscriptionRenderFrameObserver class that lives inside renderer
// process.

class AIChatIAPSubscription final : public ai_chat::mojom::IAPSubscription {
 public:
  AIChatIAPSubscription(const AIChatIAPSubscription&) = delete;
  AIChatIAPSubscription& operator=(const AIChatIAPSubscription&) = delete;
  explicit AIChatIAPSubscription(PrefService* prefs);
  ~AIChatIAPSubscription() override;

  // ai_chat::mojom::IAPSubscription
  void GetPurchaseToken(GetPurchaseTokenCallback callback) override;

 private:
  raw_ptr<PrefService> prefs_ = nullptr;
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ANDROID_AI_CHAT_IAP_SUBSCRIPTION_ANDROID_H_
