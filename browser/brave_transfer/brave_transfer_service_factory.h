// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_BRAVE_TRANSFER_BRAVE_TRANSFER_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_BRAVE_TRANSFER_BRAVE_TRANSFER_SERVICE_FACTORY_H_

#include <memory>

#include "base/no_destructor.h"
#include "chrome/browser/profiles/profile_keyed_service_factory.h"

class KeyedService;
class Profile;

namespace content {
class BrowserContext;
}  // namespace content

namespace brave_transfer {

class BraveTransferService;

// Creates the service for regular profiles only: never for incognito, Tor,
// Guest or system profiles, and not while the BraveTransfer feature is off.
class BraveTransferServiceFactory : public ProfileKeyedServiceFactory {
 public:
  static BraveTransferService* GetForProfile(Profile* profile);
  static BraveTransferServiceFactory* GetInstance();

  BraveTransferServiceFactory(const BraveTransferServiceFactory&) = delete;
  BraveTransferServiceFactory& operator=(const BraveTransferServiceFactory&) =
      delete;

 private:
  friend base::NoDestructor<BraveTransferServiceFactory>;

  BraveTransferServiceFactory();
  ~BraveTransferServiceFactory() override;

  // ProfileKeyedServiceFactory:
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
};

}  // namespace brave_transfer

#endif  // BRAVE_BROWSER_BRAVE_TRANSFER_BRAVE_TRANSFER_SERVICE_FACTORY_H_
