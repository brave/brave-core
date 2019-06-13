/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/brave_extensions_browser_client.h"

#include "brave/browser/extensions/brave_extensions_browser_api_provider.h"
#include "chrome/browser/profiles/profile.h"

namespace extensions {

BraveExtensionsBrowserClient::BraveExtensionsBrowserClient() {
  AddAPIProvider(std::make_unique<BraveExtensionsBrowserAPIProvider>());
}

BraveExtensionsBrowserClient::~BraveExtensionsBrowserClient() {
}

bool BraveExtensionsBrowserClient::HasTorContext(
    content::BrowserContext* context) {
  return static_cast<Profile*>(context)->HasTorProfile();
}

content::BrowserContext* BraveExtensionsBrowserClient::GetTorContext(
    content::BrowserContext* context) {
  return static_cast<Profile*>(context)->GetTorProfile();
}

}  // namespace extensions
