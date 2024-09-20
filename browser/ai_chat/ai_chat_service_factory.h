// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_AI_CHAT_AI_CHAT_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_AI_CHAT_AI_CHAT_SERVICE_FACTORY_H_

#include <memory>

#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "content/public/browser/browser_context.h"

namespace base {
template <typename T>
class NoDestructor;
}  // namespace base

namespace ai_chat {
class AIChatService;

class AIChatServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  AIChatServiceFactory(const AIChatServiceFactory&) = delete;
  AIChatServiceFactory& operator=(const AIChatServiceFactory&) = delete;

  static AIChatServiceFactory* GetInstance();
  static AIChatService* GetForBrowserContext(content::BrowserContext* context);

 private:
  friend base::NoDestructor<AIChatServiceFactory>;

  AIChatServiceFactory();
  ~AIChatServiceFactory() override;

  // BrowserContextKeyedServiceFactory overrides:
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_AI_CHAT_AI_CHAT_SERVICE_FACTORY_H_
