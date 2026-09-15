/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/common/scoped_chrome_extensions_client.h"

#include "base/check_is_test.h"
#include "base/command_line.h"
#include "brave/common/extensions/brave_extensions_client.h"
#include "content/public/common/content_switches.h"

namespace extensions {
namespace {

// The instance that owns the process-wide ExtensionsClient while it is being
// shared, if any. See ShouldShareProcessWideClient() below.
const ScopedChromeExtensionsClient* g_process_wide_client_owner = nullptr;

// ExtensionsClient::Set() only accepts one client per process, but with
// `--single-process` both BrowserProcessImpl and ChromeContentRendererClient
// are created in the browser process, and each one of them creates a
// ScopedChromeExtensionsClient. Share the client between them, but only in
// browser tests, which need `--single-process` (for example, to control the
// renderer's approximated device memory). Production keeps upstream's
// one-instance-per-process behavior.
bool IsSingleProcessBrowserTest() {
  const auto* const command_line = base::CommandLine::ForCurrentProcess();
  return command_line->HasSwitch(::switches::kSingleProcess) &&
         command_line->HasSwitch(::switches::kBrowserTest);
}

// Returns true if `instance` should register the process-wide client.
bool UseGlobalExtensionsClientForSingleProcessTests(
    const ScopedChromeExtensionsClient* instance) {
  if (!IsSingleProcessBrowserTest()) {
    return false;
  }
  CHECK_IS_TEST();
  if (g_process_wide_client_owner) {
    return true;
  }
  g_process_wide_client_owner = instance;
  return false;
}

// Returns true if `instance` should unregister the process-wide client.
bool ReleaseGlobalExtensionsClientForSingleProcessTests(
    const ScopedChromeExtensionsClient* instance) {
  if (!IsSingleProcessBrowserTest()) {
    return false;
  }
  CHECK_IS_TEST();
  if (g_process_wide_client_owner != instance) {
    return true;
  }
  g_process_wide_client_owner = nullptr;
  return false;
}

}  // namespace
}  // namespace extensions

#include <chrome/common/scoped_chrome_extensions_client.cc>
