/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_UNSTOPPABLE_DOMAINS_UNSTOPPABLE_DOMAINS_SERVICE_H_
#define BRAVE_COMPONENTS_UNSTOPPABLE_DOMAINS_UNSTOPPABLE_DOMAINS_SERVICE_H_

#include <memory>

#include "components/keyed_service/core/keyed_service.h"

namespace content {
class BrowserContext;
}  // namespace content

class PrefChangeRegistrar;
class PrefRegistrySimple;
class PrefService;

namespace unstoppable_domains {

class UnstoppableDomainsServiceDelegate;

class UnstoppableDomainsService : public KeyedService {
 public:
  UnstoppableDomainsService(
      std::unique_ptr<UnstoppableDomainsServiceDelegate> delegate,
      content::BrowserContext* context,
      PrefService* local_state);
  ~UnstoppableDomainsService() override;

  UnstoppableDomainsService(const UnstoppableDomainsService&) = delete;
  UnstoppableDomainsService& operator=(const UnstoppableDomainsService&) =
      delete;

  static void RegisterLocalStatePrefs(PrefRegistrySimple* registry);

 private:
  void OnPreferenceChanged();

  std::unique_ptr<PrefChangeRegistrar> pref_change_registrar_;
  std::unique_ptr<UnstoppableDomainsServiceDelegate> delegate_;
};

}  // namespace unstoppable_domains

#endif  // BRAVE_COMPONENTS_UNSTOPPABLE_DOMAINS_UNSTOPPABLE_DOMAINS_SERVICE_H_
