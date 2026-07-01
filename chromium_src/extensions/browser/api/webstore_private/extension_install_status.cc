/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "extensions/browser/api/webstore_private/extension_install_status.h"

#include "extensions/browser/manifest_v2_handler.h"

// Pass extension id as an additional parameter to
// mv2_handler->ShouldBlockExtensionInstallation so that the extension can
// be checked against the Brave-hosted list.
#define ShouldBlockExtensionInstallation(...) \
  ShouldBlockExtensionInstallation(extension_id, __VA_ARGS__)

#include <extensions/browser/api/webstore_private/extension_install_status.cc>
#undef ShouldBlockExtensionInstallation
