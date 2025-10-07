/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/content/browser/ollama_client_factory.h"

#include "base/check.h"
#include "base/no_destructor.h"
#include "brave/components/ai_chat/core/browser/ollama/ollama_client.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"

namespace ai_chat {

// static
OllamaClientFactory* OllamaClientFactory::GetInstance() {
  static base::NoDestructor<OllamaClientFactory> instance;
  return instance.get();
}

// static
OllamaClient* OllamaClientFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  DCHECK(context);
  if (features::IsAIChatEnabled()) {
    return static_cast<OllamaClient*>(
        GetInstance()->GetServiceForBrowserContext(context, true));
  }

  return nullptr;
}

OllamaClientFactory::OllamaClientFactory()
    : BrowserContextKeyedServiceFactory(
          "OllamaClientFactory",
          BrowserContextDependencyManager::GetInstance()) {}

OllamaClientFactory::~OllamaClientFactory() = default;

std::unique_ptr<KeyedService>
OllamaClientFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  auto url_loader_factory =
      context->GetDefaultStoragePartition()
          ->GetURLLoaderFactoryForBrowserProcess();
  return std::make_unique<OllamaClient>(url_loader_factory);
}

}  // namespace ai_chat
