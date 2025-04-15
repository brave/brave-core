/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/brave_extension_service.h"
#include "chrome/browser/extensions/crx_installer.h"
#include "extensions/browser/update_install_gate.h"

#define ExtensionService BraveExtensionService
#include "src/chrome/browser/extensions/chrome_extension_system.cc"
#undef ExtensionService
