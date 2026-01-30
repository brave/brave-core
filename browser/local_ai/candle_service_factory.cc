// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/local_ai/candle_service_factory.h"

#include <memory>

#include "base/feature_list.h"
#include "base/no_destructor.h"
#include "brave/components/local_ai/browser/candle_service.h"
#include "brave/components/local_ai/common/features.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "content/public/browser/browser_context.h"

namespace local_ai {

// static
CandleServiceFactory* CandleServiceFactory::GetInstance() {
  static base::NoDestructor<CandleServiceFactory> instance;
  return instance.get();
}

CandleService* CandleServiceFactory::GetForBrowserContext(
    content::BrowserContext* browser_context) {
  return static_cast<CandleService*>(
      CandleServiceFactory::GetInstance()->GetServiceForBrowserContext(
          browser_context, true /*create*/));
}

CandleServiceFactory::CandleServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "CandleService",
          BrowserContextDependencyManager::GetInstance()) {}

CandleServiceFactory::~CandleServiceFactory() = default;

content::BrowserContext* CandleServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return GetBrowserContextRedirectedInIncognito(context);
}

std::unique_ptr<KeyedService>
CandleServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  if (!base::FeatureList::IsEnabled(features::kLocalAIModels)) {
    return nullptr;
  }

  return std::make_unique<CandleService>();
}

}  // namespace local_ai
