/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/misc_metrics/extension_metrics_service_factory.h"

#include "base/no_destructor.h"
#include "brave/browser/misc_metrics/extension_metrics_service.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "extensions/browser/extension_registry_factory.h"

namespace misc_metrics {

// static
ExtensionMetricsServiceFactory* ExtensionMetricsServiceFactory::GetInstance() {
  static base::NoDestructor<ExtensionMetricsServiceFactory> instance;
  return instance.get();
}

// static
ExtensionMetricsService* ExtensionMetricsServiceFactory::GetServiceForContext(
    content::BrowserContext* context) {
  return static_cast<ExtensionMetricsService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

ExtensionMetricsServiceFactory::ExtensionMetricsServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "ExtensionMetricsService",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(extensions::ExtensionRegistryFactory::GetInstance());
}

ExtensionMetricsServiceFactory::~ExtensionMetricsServiceFactory() = default;

KeyedService* ExtensionMetricsServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  auto* extension_registry =
      extensions::ExtensionRegistryFactory::GetForBrowserContext(context);
  return new ExtensionMetricsService(extension_registry);
}

content::BrowserContext* ExtensionMetricsServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  if (context->IsOffTheRecord() ||
      !extensions::ExtensionRegistryFactory::GetForBrowserContext(context)) {
    return nullptr;
  }
  return context;
}

}  // namespace misc_metrics
