/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_VPN_BRAVE_VPN_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_BRAVE_VPN_BRAVE_VPN_SERVICE_FACTORY_H_

#include <memory>

#include "brave/components/brave_vpn/common/mojom/brave_vpn.mojom.h"
#include "build/build_config.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#if BUILDFLAG(IS_ANDROID)
#include "mojo/public/cpp/bindings/pending_remote.h"
#endif  // BUILDFLAG(IS_ANDROID)

class Profile;

namespace base {
template <typename T>
class NoDestructor;
}  // namespace base

namespace brave_vpn {

class BraveVpnService;

class BraveVpnServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  static BraveVpnService* GetForProfile(Profile* profile);
#if BUILDFLAG(IS_ANDROID)
  static mojo::PendingRemote<brave_vpn::mojom::ServiceHandler> GetForContext(
      content::BrowserContext* context);
#endif  // BUILDFLAG(IS_ANDROID)
  static BraveVpnServiceFactory* GetInstance();

  BraveVpnServiceFactory(const BraveVpnServiceFactory&) = delete;
  BraveVpnServiceFactory& operator=(const BraveVpnServiceFactory&) = delete;

  static void BindForContext(
      content::BrowserContext* context,
      mojo::PendingReceiver<brave_vpn::mojom::ServiceHandler> receiver);

  // Returns the default factory, useful in tests.
  static TestingFactory GetDefaultFactory();

 private:
  friend base::NoDestructor<BraveVpnServiceFactory>;

  BraveVpnServiceFactory();
  ~BraveVpnServiceFactory() override;

  // BrowserContextKeyedServiceFactory overrides:
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
  void RegisterProfilePrefs(
      user_prefs::PrefRegistrySyncable* registry) override;
};

}  // namespace brave_vpn

#endif  // BRAVE_BROWSER_BRAVE_VPN_BRAVE_VPN_SERVICE_FACTORY_H_
