/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_talk/brave_talk_tab_capture_registry_factory.h"

#include <memory>
#include <utility>

#include "brave/browser/brave_talk/brave_talk_tab_capture_registry.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

namespace brave_talk {

// static
BraveTalkTabCaptureRegistryFactory*
BraveTalkTabCaptureRegistryFactory::GetInstance() {
  return base::Singleton<BraveTalkTabCaptureRegistryFactory>::get();
}

BraveTalkTabCaptureRegistry* BraveTalkTabCaptureRegistryFactory::GetForContext(
    content::BrowserContext* context) {
  return static_cast<BraveTalkTabCaptureRegistry*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

BraveTalkTabCaptureRegistryFactory::BraveTalkTabCaptureRegistryFactory()
    : BrowserContextKeyedServiceFactory(
          "BraveTalkTabCaptureRegistry",
          BrowserContextDependencyManager::GetInstance()) {}

BraveTalkTabCaptureRegistryFactory::~BraveTalkTabCaptureRegistryFactory() =
    default;

KeyedService* BraveTalkTabCaptureRegistryFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new BraveTalkTabCaptureRegistry();
}

}  // namespace brave_talk
