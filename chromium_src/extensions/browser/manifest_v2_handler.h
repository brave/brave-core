/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_EXTENSIONS_BROWSER_MANIFEST_V2_HANDLER_H_
#define BRAVE_CHROMIUM_SRC_EXTENSIONS_BROWSER_MANIFEST_V2_HANDLER_H_

// This override modifies ShouldBlockExtensionInstallation method's arguments to
// allow for checking the given extension agains Brave-hosted extension list.
#define ShouldBlockExtensionInstallation(...) \
  ShouldBlockExtensionInstallation(const ExtensionId& extension_id, __VA_ARGS__)

#include <extensions/browser/manifest_v2_handler.h>  // IWYU pragma: export
#undef ShouldBlockExtensionInstallation

#endif  // BRAVE_CHROMIUM_SRC_EXTENSIONS_BROWSER_MANIFEST_V2_HANDLER_H_
