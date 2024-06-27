/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_AI_CHAT_SERVICE_FACTORY_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_AI_CHAT_SERVICE_FACTORY_H_

#include <memory>

#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace base {
template <typename T>
class NoDestructor;
}  // namespace base

namespace ai_chat {
class AIChatKeyedService;

class AIChatServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  AIChatServiceFactory(const AIChatServiceFactory&) = delete;
  AIChatServiceFactory& operator=(const AIChatServiceFactory&) = delete;

  static AIChatServiceFactory* GetInstance();
  static AIChatKeyedService* GetForBrowserContext(
      content::BrowserContext* context);

 private:
  friend base::NoDestructor<AIChatServiceFactory>;

  AIChatServiceFactory();
  ~AIChatServiceFactory() override;

  // BrowserContextKeyedServiceFactory overrides:
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_AI_CHAT_SERVICE_FACTORY_H_
