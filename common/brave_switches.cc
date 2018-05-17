/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/common/brave_switches.h"

namespace switches {

// Allows disabling the Brave extension.
// This is commonly used for loading the extension manually to debug things
// in debug mode with auto-reloading.
const char kDisableBraveExtension[] = "disable-brave-extension";

// This switch enables update module(Sparkle).
// When you use this flag for update test, make sure to fix the feed URL
// |brave_feed_url| in brave/brave_init_settings.gni.
// This switch is introduced for update feature development.
// When update test is fininshed, update module should be enabled only in
// official build.
// TODO(shong): Remove this switch when update development is done.
const char kEnableBraveUpdateTest[] = "enable-brave-update-test";

}  // namespace switches
