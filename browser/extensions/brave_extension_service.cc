/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/brave_extension_service.h"

#include "chrome/browser/extensions/extension_service.h"
#include "extensions/browser/blocklist.h"
#include "extensions/browser/extension_system.h"

namespace extensions {

BraveExtensionService::BraveExtensionService(ExtensionSystem* extension_system)
    : extension_system_(extension_system) {
  if (auto* blocklist = extension_malware_blocklist::ExtensionMalwareBlocklist::
          GetInstance()) {
    observation_.Observe(blocklist);
    // If the list already loaded before this profile came up, catch up now.
    if (blocklist->is_ready()) {
      OnMalwareListUpdated();
    }
  }
}

BraveExtensionService::~BraveExtensionService() = default;

void BraveExtensionService::Shutdown() {
  // The observed blocklist is a process-wide holder that outlives this
  // profile-scoped service, so stop observing during ordered teardown rather
  // than waiting for destruction.
  observation_.Reset();
}

void BraveExtensionService::OnMalwareListUpdated() {
  ExtensionService* extension_service = extension_system_->extension_service();
  if (extension_service) {
    // OnBlocklistUpdated() is private on ExtensionService but public on the
    // Blocklist::Observer interface it implements, so re-trigger enforcement
    // through the base interface instead of befriending ExtensionService.
    static_cast<Blocklist::Observer*>(extension_service)->OnBlocklistUpdated();
  }
}

}  // namespace extensions
