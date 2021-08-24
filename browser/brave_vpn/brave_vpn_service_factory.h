/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_VPN_BRAVE_VPN_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_BRAVE_VPN_BRAVE_VPN_SERVICE_FACTORY_H_

#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

class Profile;

#if defined(OS_WIN) || defined(OS_MAC)
class BraveVpnServiceDesktop;
#endif

#if defined(OS_ANDROID)
class BraveVpnService;
#endif

class BraveVpnServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
#if defined(OS_WIN) || defined(OS_MAC)
  static BraveVpnServiceDesktop* GetForProfile(Profile* profile);
#endif

#if defined(OS_ANDROID)
  static BraveVpnService* GetForProfile(Profile* profile);
#endif
  static BraveVpnServiceFactory* GetInstance();

  BraveVpnServiceFactory(const BraveVpnServiceFactory&) = delete;
  BraveVpnServiceFactory& operator=(const BraveVpnServiceFactory&) = delete;

 private:
  friend struct base::DefaultSingletonTraits<BraveVpnServiceFactory>;

  BraveVpnServiceFactory();
  ~BraveVpnServiceFactory() override;

  // BrowserContextKeyedServiceFactory overrides:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
};

#endif  // BRAVE_BROWSER_BRAVE_VPN_BRAVE_VPN_SERVICE_FACTORY_H_
