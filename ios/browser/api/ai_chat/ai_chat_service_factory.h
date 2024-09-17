// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_API_AI_CHAT_AI_CHAT_SERVICE_FACTORY_H_
#define BRAVE_IOS_BROWSER_API_AI_CHAT_AI_CHAT_SERVICE_FACTORY_H_

#include <memory>

#include "base/no_destructor.h"
#include "brave/components/ai_chat/core/browser/ai_chat_metrics.h"
#include "components/keyed_service/ios/browser_state_keyed_service_factory.h"

class ChromeBrowserState;

namespace base {

template <typename T>
class NoDestructor;
}  // namespace base

namespace ai_chat {

class AIChatMetrics;
class AIChatService;

class AIChatServiceFactory : public BrowserStateKeyedServiceFactory {
 public:
  static AIChatService* GetForBrowserState(ChromeBrowserState* browser_state);
  static AIChatServiceFactory* GetInstance();

  AIChatServiceFactory(const AIChatServiceFactory&) = delete;
  AIChatServiceFactory& operator=(const AIChatServiceFactory&) = delete;

 private:
  friend class base::NoDestructor<AIChatServiceFactory>;

  AIChatServiceFactory();
  ~AIChatServiceFactory() override;

  // BrowserStateKeyedServiceFactory implementation.
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      web::BrowserState* context) const override;
  web::BrowserState* GetBrowserStateToUse(
      web::BrowserState* context) const override;
  bool ServiceIsNULLWhileTesting() const override;

  std::unique_ptr<AIChatMetrics> ai_chat_metrics_;
};
}  // namespace ai_chat

#endif  // BRAVE_IOS_BROWSER_API_AI_CHAT_AI_CHAT_SERVICE_FACTORY_H_
