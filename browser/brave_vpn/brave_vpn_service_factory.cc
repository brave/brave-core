/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_vpn/brave_vpn_service_factory.h"

#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"

#if defined(OS_WIN) || defined(OS_MAC)
#include "brave/browser/skus/sdk_controller_factory.h"
#include "brave/components/brave_vpn/brave_vpn_service_desktop.h"
#include "brave/components/brave_vpn/brave_vpn_utils.h"
#endif

#if defined(OS_ANDROID)
#include "brave/components/brave_vpn/brave_vpn_service.h"
#endif

// static
BraveVpnServiceFactory* BraveVpnServiceFactory::GetInstance() {
  return base::Singleton<BraveVpnServiceFactory>::get();
}

#if defined(OS_WIN) || defined(OS_MAC)
// static
BraveVpnServiceDesktop* BraveVpnServiceFactory::GetForProfile(
    Profile* profile) {
  if (!brave_vpn::IsBraveVPNEnabled())
    return nullptr;

  return static_cast<BraveVpnServiceDesktop*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}
#endif

#if defined(OS_ANDROID)
// static
BraveVpnService* BraveVpnServiceFactory::GetForProfile(Profile* profile) {
  return static_cast<BraveVpnService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}
#endif

BraveVpnServiceFactory::BraveVpnServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "BraveVpnService",
          BrowserContextDependencyManager::GetInstance()) {
#if defined(OS_WIN) || defined(OS_MAC)
  DependsOn(skus::SdkControllerFactory::GetInstance());
#endif
}

BraveVpnServiceFactory::~BraveVpnServiceFactory() = default;

KeyedService* BraveVpnServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  auto* default_storage_partition = context->GetDefaultStoragePartition();
  auto shared_url_loader_factory =
      default_storage_partition->GetURLLoaderFactoryForBrowserProcess();

#if defined(OS_WIN) || defined(OS_MAC)
  return new BraveVpnServiceDesktop(
      shared_url_loader_factory, user_prefs::UserPrefs::Get(context),
      skus::SdkControllerFactory::GetControllerForContext(context));
#endif

#if defined(OS_ANDROID)
  return new BraveVpnService(shared_url_loader_factory);
#endif
}

content::BrowserContext* BraveVpnServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}
