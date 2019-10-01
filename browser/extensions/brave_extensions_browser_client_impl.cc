/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/brave_extensions_browser_client_impl.h"

#include <memory>

#include "brave/browser/extensions/brave_extensions_browser_api_provider.h"
#include "brave/browser/profiles/profile_util.h"
#include "components/prefs/pref_service.h"

namespace extensions {

BraveExtensionsBrowserClientImpl::BraveExtensionsBrowserClientImpl() {
  BraveExtensionsBrowserClient::Set(this);
  AddAPIProvider(std::make_unique<BraveExtensionsBrowserAPIProvider>());
}

}  // namespace extensions
