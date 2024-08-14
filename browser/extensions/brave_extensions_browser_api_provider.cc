/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/brave_extensions_browser_api_provider.h"

#include "brave/browser/extensions/api/generated_api_registration.h"
#include "extensions/browser/extension_function_registry.h"

namespace extensions {

BraveExtensionsBrowserAPIProvider::BraveExtensionsBrowserAPIProvider() =
    default;
BraveExtensionsBrowserAPIProvider::~BraveExtensionsBrowserAPIProvider() =
    default;

void BraveExtensionsBrowserAPIProvider::RegisterExtensionFunctions(
    ExtensionFunctionRegistry* registry) {
  // Generated APIs from Brave.
  api::BraveGeneratedFunctionRegistry::RegisterAll(registry);
}

}  // namespace extensions
