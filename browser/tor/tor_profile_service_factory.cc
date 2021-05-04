/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/tor/tor_profile_service_factory.h"

#include <memory>

#include "brave/browser/brave_browser_process.h"
#include "brave/components/tor/pref_names.h"
#include "brave/components/tor/tor_profile_service_impl.h"
#include "chrome/browser/browser_process.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_context.h"

// static
tor::TorProfileService* TorProfileServiceFactory::GetForContext(
    content::BrowserContext* context) {
  return GetForContext(context, true);
}

// static
tor::TorProfileService* TorProfileServiceFactory::GetForContext(
    content::BrowserContext* context,
    bool create) {
  return static_cast<tor::TorProfileService*>(
      GetInstance()->GetServiceForBrowserContext(context, create));
}

// static
TorProfileServiceFactory* TorProfileServiceFactory::GetInstance() {
  return base::Singleton<TorProfileServiceFactory>::get();
}

// static
void TorProfileServiceFactory::SetTorDisabled(bool disabled) {
  if (g_browser_process)
    g_browser_process->local_state()->SetBoolean(tor::prefs::kTorDisabled,
                                                 disabled);
}

// static
bool TorProfileServiceFactory::IsTorDisabled() {
  if (g_browser_process)
    return g_browser_process->local_state()->GetBoolean(
        tor::prefs::kTorDisabled);
  return false;
}

TorProfileServiceFactory::TorProfileServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "TorProfileService",
          BrowserContextDependencyManager::GetInstance()) {}

TorProfileServiceFactory::~TorProfileServiceFactory() {}

KeyedService* TorProfileServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  std::unique_ptr<tor::TorProfileService> tor_profile_service(
      new tor::TorProfileServiceImpl(
          context, g_brave_browser_process
                       ? g_brave_browser_process->tor_client_updater()
                       : nullptr));

  return tor_profile_service.release();
}

content::BrowserContext* TorProfileServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  // Only grant service for tor context
  if (!context->IsTor())
    return nullptr;
  return context;
}
