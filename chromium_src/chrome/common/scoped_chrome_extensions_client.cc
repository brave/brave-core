/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/common/scoped_chrome_extensions_client.h"

#include "brave/common/extensions/brave_extensions_client.h"

namespace extensions {
namespace {

// The instance that owns the process-wide ExtensionsClient, if any. With
// `--single-process` both BrowserProcessImpl and ChromeContentRendererClient
// are created in the browser process, and each one of them creates a
// ScopedChromeExtensionsClient, whereas ExtensionsClient::Set() only accepts
// one client per process.
const ScopedChromeExtensionsClient* g_process_wide_client_owner = nullptr;

}  // namespace
}  // namespace extensions

#include <chrome/common/scoped_chrome_extensions_client.cc>
