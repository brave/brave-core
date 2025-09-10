// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/local_ai/fast_vlm_service_factory.h"

#include <memory>

#include "base/no_destructor.h"
#include "brave/browser/local_ai/fast_vlm_service.h"
#include "chrome/browser/profiles/profile_selections.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "content/public/browser/browser_context.h"

namespace local_ai {

// static
FastVLMServiceFactory* FastVLMServiceFactory::GetInstance() {
  static base::NoDestructor<FastVLMServiceFactory> instance;
  return instance.get();
}

// static
FastVLMService* FastVLMServiceFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<FastVLMService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

FastVLMServiceFactory::FastVLMServiceFactory()
    : ProfileKeyedServiceFactory(
          "FastVLMService",
          ProfileSelections::Builder()
              .WithRegular(ProfileSelection::kOriginalOnly)
              .Build()) {
  // No dependencies for now - could add dependencies on other services
}

FastVLMServiceFactory::~FastVLMServiceFactory() = default;

std::unique_ptr<KeyedService>
FastVLMServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  auto service = std::make_unique<FastVLMService>(context);

  // Defer initialization to avoid blocking main thread during service creation
  // The service will initialize lazily when first accessed

  return service;
}

}  // namespace local_ai
