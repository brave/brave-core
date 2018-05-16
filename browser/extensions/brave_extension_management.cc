/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/brave_extension_management.h"

#include "brave/extensions/browser/brave_extension_provider.h"

namespace extensions {

BraveExtensionManagement::BraveExtensionManagement(
    PrefService* pref_service,
    bool is_signin_profile)
    : ExtensionManagement(pref_service, is_signin_profile) {
  providers_.push_back(
      std::make_unique<BraveExtensionProvider>());
}

BraveExtensionManagement::~BraveExtensionManagement() {
}

}  // namespace extensions
