/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/common/brave_switches.h"

namespace switches {

// Allows disabling the Brave extension.
// This is commonly used for loading the extension manually to debug things
// in debug mode with auto-reloading.
const char kDisableBraveExtension[] = "disable-brave-extension";

// Trigger update check.
// Update check only should be done in official build.
// For update feature development, update check is enabled by this swtich.
// TODO(shong): Remove when update development is done.
const char kEnableBraveUpdateTest[] = "enable-brave-update-test";

}  // namespace switches
