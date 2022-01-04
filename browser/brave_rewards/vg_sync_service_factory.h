/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_REWARDS_VG_SYNC_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_BRAVE_REWARDS_VG_SYNC_SERVICE_FACTORY_H_

#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

class Profile;
class VgSyncService;

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}

class VgSyncServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  VgSyncServiceFactory(const VgSyncServiceFactory&) = delete;
  VgSyncServiceFactory& operator=(const VgSyncServiceFactory&) = delete;

  static VgSyncServiceFactory* GetInstance();

  static VgSyncService* GetForProfile(Profile* profile);

 private:
  friend struct base::DefaultSingletonTraits<VgSyncServiceFactory>;

  VgSyncServiceFactory();
  ~VgSyncServiceFactory() override;

  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
};
#endif  // BRAVE_BROWSER_BRAVE_REWARDS_VG_SYNC_SERVICE_FACTORY_H_
