/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
 
#include "brave/browser/brave_talk/brave_talk_service_factory.h"

#include <memory>
#include <utility>

#include "brave/browser/brave_talk/brave_talk_service.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

namespace brave_talk {

// static
BraveTalkServiceFactory* BraveTalkServiceFactory::GetInstance() {
  return base::Singleton<BraveTalkServiceFactory>::get();
}

BraveTalkService* BraveTalkServiceFactory::GetForContext(
    content::BrowserContext* context) {
  return static_cast<BraveTalkService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

BraveTalkServiceFactory::BraveTalkServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "BraveTalkService",
          BrowserContextDependencyManager::GetInstance()) {}

BraveTalkServiceFactory::~BraveTalkServiceFactory() = default;

KeyedService* BraveTalkServiceFactory::BuildServiceInstanceFor(content::BrowserContext *context) const {
    return new BraveTalkService();
}

}  // namespace brave_talk