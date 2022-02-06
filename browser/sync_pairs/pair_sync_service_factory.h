/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_SYNC_PAIRS_PAIR_SYNC_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_SYNC_PAIRS_PAIR_SYNC_SERVICE_FACTORY_H_

#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

class PairSyncService;
class Profile;

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}

class PairSyncServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  PairSyncServiceFactory(const PairSyncServiceFactory&) = delete;
  PairSyncServiceFactory& operator=(const PairSyncServiceFactory&) = delete;

  static PairSyncServiceFactory* GetInstance();

  static PairSyncService* GetForProfile(Profile* profile);

 private:
  friend struct base::DefaultSingletonTraits<PairSyncServiceFactory>;

  PairSyncServiceFactory();
  ~PairSyncServiceFactory() override;

  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
};
#endif  // BRAVE_BROWSER_SYNC_PAIRS_PAIR_SYNC_SERVICE_FACTORY_H_
