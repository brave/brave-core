/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_BRAVE_WALLET_MELD_INTEGRATION_SERVICE_FACTORY_H_
#define BRAVE_IOS_BROWSER_BRAVE_WALLET_MELD_INTEGRATION_SERVICE_FACTORY_H_

#include <memory>

#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "components/keyed_service/ios/browser_state_keyed_service_factory.h"
#include "mojo/public/cpp/bindings/pending_remote.h"

class ProfileIOS;
class KeyedService;

namespace base {
template <typename T>
class NoDestructor;
}  // namespace base

namespace web {
class BrowserState;
}  // namespace web

namespace brave_wallet {

class MeldIntegrationService;

class MeldIntegrationServiceFactory : public BrowserStateKeyedServiceFactory {
 public:
  // Creates the service if it doesn't exist already for |profile|.
  static mojo::PendingRemote<mojom::MeldIntegrationService> GetForBrowserState(
      ProfileIOS* profile);
  static MeldIntegrationService* GetServiceForState(ProfileIOS* profile);

  static MeldIntegrationServiceFactory* GetInstance();

 private:
  friend base::NoDestructor<MeldIntegrationServiceFactory>;

  MeldIntegrationServiceFactory();
  ~MeldIntegrationServiceFactory() override;

  // BrowserContextKeyedServiceFactory:
  // BrowserStateKeyedServiceFactory implementation.
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      web::BrowserState* context) const override;
  bool ServiceIsNULLWhileTesting() const override;
  web::BrowserState* GetBrowserStateToUse(
      web::BrowserState* context) const override;

  MeldIntegrationServiceFactory(const MeldIntegrationServiceFactory&) = delete;
  MeldIntegrationServiceFactory& operator=(
      const MeldIntegrationServiceFactory&) = delete;
};

}  // namespace brave_wallet

#endif  // BRAVE_IOS_BROWSER_BRAVE_WALLET_MELD_INTEGRATION_SERVICE_FACTORY_H_
