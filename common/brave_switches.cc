/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/common/brave_switches.h"

namespace switches {

// Allows disabling the Brave extension.
// This is commonly used for loading the extension manually to debug things
// in debug mode with auto-reloading.
const char kDisableBraveExtension[] = "disable-brave-extension";

// Allows disabling the PDFJS extension.
const char kDisablePDFJSExtension[] = "disable-pdfjs-extension";

// This switch disables update module(Sparkle).
const char kDisableBraveUpdate[] = "disable-brave-update";

}  // namespace switches
