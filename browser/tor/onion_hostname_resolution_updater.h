/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_TOR_ONION_HOSTNAME_RESOLUTION_UPDATER_H_
#define BRAVE_BROWSER_TOR_ONION_HOSTNAME_RESOLUTION_UPDATER_H_

#include "base/memory/raw_ptr.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/prefs/pref_change_registrar.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace tor {

// Pushes changes of the "Only resolve .onion addresses in Tor windows" pref to
// the network context of every loaded storage partition. Network contexts
// created later get the current value from their creation params.
class OnionHostnameResolutionUpdater : public KeyedService {
 public:
  explicit OnionHostnameResolutionUpdater(content::BrowserContext* context);
  OnionHostnameResolutionUpdater(const OnionHostnameResolutionUpdater&) =
      delete;
  OnionHostnameResolutionUpdater& operator=(
      const OnionHostnameResolutionUpdater&) = delete;
  ~OnionHostnameResolutionUpdater() override;

  // KeyedService:
  void Shutdown() override;

 private:
  void UpdateNetworkContexts();

  PrefChangeRegistrar pref_change_registrar_;
  raw_ptr<content::BrowserContext> context_;
};

}  // namespace tor

#endif  // BRAVE_BROWSER_TOR_ONION_HOSTNAME_RESOLUTION_UPDATER_H_
