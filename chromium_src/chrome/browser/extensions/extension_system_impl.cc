/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/brave_extension_service.h"
#include "chrome/browser/extensions/crx_installer.h"
#include "chrome/browser/extensions/update_install_gate.h"

#define ExtensionService BraveExtensionService
#include "src/chrome/browser/extensions/extension_system_impl.cc"
#undef ExtensionService
