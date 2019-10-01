/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/extensions/browser/brave_extensions_browser_client.h"

#include <stddef.h>

namespace extensions {

namespace {

BraveExtensionsBrowserClient* g_brave_extension_browser_client = NULL;

}  // namespace

BraveExtensionsBrowserClient::BraveExtensionsBrowserClient() {}
BraveExtensionsBrowserClient::~BraveExtensionsBrowserClient() = default;

BraveExtensionsBrowserClient* BraveExtensionsBrowserClient::Get() {
  return g_brave_extension_browser_client;
}

void BraveExtensionsBrowserClient::Set(BraveExtensionsBrowserClient* client) {
  g_brave_extension_browser_client = client;
}

}  // namespace exceptions
