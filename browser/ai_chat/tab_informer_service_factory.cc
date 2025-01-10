// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/tab_informer_service_factory.h"

#include <memory>

#include "brave/components/ai_chat/core/browser/tab_informer_service.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

namespace ai_chat {

// static
TabInformerServiceFactory* TabInformerServiceFactory::GetInstance() {
  static base::NoDestructor<TabInformerServiceFactory> instance;
  return instance.get();
}

// static
TabInformerService* TabInformerServiceFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  CHECK(context);

  return static_cast<TabInformerService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

TabInformerServiceFactory::TabInformerServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "TabInformerService",
          BrowserContextDependencyManager::GetInstance()) {}

TabInformerServiceFactory::~TabInformerServiceFactory() = default;

bool TabInformerServiceFactory::ServiceIsCreatedWithBrowserContext() const {
  return true;
}

std::unique_ptr<KeyedService>
TabInformerServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  return std::make_unique<TabInformerService>();
}

}  // namespace ai_chat
