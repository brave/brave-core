// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_OLLAMA_CLIENT_FACTORY_H_
#define BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_OLLAMA_CLIENT_FACTORY_H_

#include <memory>

#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "components/keyed_service/core/keyed_service.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace base {

template <typename T>
class NoDestructor;
}  // namespace base

namespace ai_chat {

class OllamaClient;

class OllamaClientFactory : public BrowserContextKeyedServiceFactory {
 public:
  OllamaClientFactory(const OllamaClientFactory&) = delete;
  OllamaClientFactory& operator=(const OllamaClientFactory&) = delete;

  static OllamaClientFactory* GetInstance();
  static OllamaClient* GetForBrowserContext(content::BrowserContext* context);

 private:
  friend base::NoDestructor<OllamaClientFactory>;

  OllamaClientFactory();
  ~OllamaClientFactory() override;

  // BrowserContextKeyedServiceFactory overrides:
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
};
}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_OLLAMA_CLIENT_FACTORY_H_
