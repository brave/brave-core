/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_EXTENSIONS_BRAVE_EXTENSION_SERVICE_H_
#define BRAVE_BROWSER_EXTENSIONS_BRAVE_EXTENSION_SERVICE_H_

#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "brave/components/extension_malware_blocklist/browser/extension_malware_blocklist.h"
#include "components/keyed_service/core/keyed_service.h"

namespace extensions {

class ExtensionSystem;

// Re-runs the per-profile extension blocklist evaluation whenever the local
// malicious-extension list loads or updates. Without this, enforcement only
// fires on the upstream blocklist database updates or at startup, so a list
// update never takes effect on its own and nothing happens when that database
// is disabled.
class BraveExtensionService
    : public KeyedService,
      public extension_malware_blocklist::ExtensionMalwareBlocklist::Observer {
 public:
  explicit BraveExtensionService(ExtensionSystem* extension_system);
  BraveExtensionService(const BraveExtensionService&) = delete;
  BraveExtensionService& operator=(const BraveExtensionService&) = delete;
  ~BraveExtensionService() override;

  // KeyedService:
  void Shutdown() override;

  // extension_malware_blocklist::ExtensionMalwareBlocklist::Observer:
  void OnMalwareListUpdated() override;

 private:
  const raw_ptr<ExtensionSystem> extension_system_;

  base::ScopedObservation<
      extension_malware_blocklist::ExtensionMalwareBlocklist,
      extension_malware_blocklist::ExtensionMalwareBlocklist::Observer>
      observation_{this};
};

}  // namespace extensions

#endif  // BRAVE_BROWSER_EXTENSIONS_BRAVE_EXTENSION_SERVICE_H_
