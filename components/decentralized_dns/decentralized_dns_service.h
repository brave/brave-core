/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_DECENTRALIZED_DNS_DECENTRALIZED_DNS_SERVICE_H_
#define BRAVE_COMPONENTS_DECENTRALIZED_DNS_DECENTRALIZED_DNS_SERVICE_H_

#include <memory>

#include "components/keyed_service/core/keyed_service.h"

namespace content {
class BrowserContext;
}  // namespace content

class PrefChangeRegistrar;
class PrefRegistrySimple;
class PrefService;

namespace decentralized_dns {

class DecentralizedDnsServiceDelegate;

class DecentralizedDnsService : public KeyedService {
 public:
  DecentralizedDnsService(
      std::unique_ptr<DecentralizedDnsServiceDelegate> delegate,
      content::BrowserContext* context,
      PrefService* local_state);
  ~DecentralizedDnsService() override;

  DecentralizedDnsService(const DecentralizedDnsService&) = delete;
  DecentralizedDnsService& operator=(const DecentralizedDnsService&) = delete;

  static void RegisterLocalStatePrefs(PrefRegistrySimple* registry);

 private:
  void OnPreferenceChanged();

  std::unique_ptr<PrefChangeRegistrar> pref_change_registrar_;
  std::unique_ptr<DecentralizedDnsServiceDelegate> delegate_;
};

}  // namespace decentralized_dns

#endif  // BRAVE_COMPONENTS_DECENTRALIZED_DNS_DECENTRALIZED_DNS_SERVICE_H_
