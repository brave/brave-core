/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/windows_recall/windows_recall_service_factory.h"

#include "base/no_destructor.h"
#include "brave/components/windows_recall/windows_recall.h"
#include "brave/components/windows_recall/windows_recall_service.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"

namespace windows_recall {

// static
WindowsRecallServiceFactory* WindowsRecallServiceFactory::GetInstance() {
  static base::NoDestructor<WindowsRecallServiceFactory> instance;
  return instance.get();
}

// static
WindowsRecallService* WindowsRecallServiceFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  if (!IsWindowsRecallAvailable()) {
    return nullptr;
  }
  return static_cast<WindowsRecallService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

WindowsRecallServiceFactory::WindowsRecallServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "WindowsRecallServiceFactory",
          BrowserContextDependencyManager::GetInstance()) {}

WindowsRecallServiceFactory::~WindowsRecallServiceFactory() = default;

content::BrowserContext* WindowsRecallServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  if (context->IsOffTheRecord()) {
    return nullptr;
  }
  return context;
}

std::unique_ptr<KeyedService>
WindowsRecallServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  return std::make_unique<WindowsRecallService>(
      user_prefs::UserPrefs::Get(context));
}

}  // namespace windows_recall
