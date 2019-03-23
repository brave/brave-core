/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/common/brave_switches.h"

namespace switches {

// Allows disabling the Brave extension.
// This is commonly used for loading the extension manually to debug things
// in debug mode with auto-reloading.
const char kDisableBraveExtension[] = "disable-brave-extension";

// Allows disabling the Brave Rewards extension.
const char kDisableBraveRewardsExtension[] = "disable-brave-rewards-extension";

// This switch disables update module(Sparkle).
const char kDisableBraveUpdate[] = "disable-brave-update";

// This switch disables the ChromeGoogleURLTrackerClient
const char kDisableChromeGoogleURLTrackingClient[] =
  "disable-chrome-google-url-tracking-client";

// Allows disabling the PDFJS extension.
const char kDisablePDFJSExtension[] = "disable-pdfjs-extension";

// Allows disabling the Tor client updater extension.
const char kDisableTorClientUpdaterExtension[] =
  "disable-tor-client-updater-extension";

// Allows disabling the WebTorrent extension.
const char kDisableWebTorrentExtension[] = "disable-webtorrent-extension";

// Allows disabling Brave Sync.
const char kDisableBraveSync[] = "disable-brave-sync";

// Specifies overriding the built-in theme setting.
// Valid values are: "dark" | "light".
const char kUiMode[] = "ui-mode";

// Triggers auto-import of profile data from Brave browser-laptop/Muon, if
// available.
const char kUpgradeFromMuon[] = "upgrade-from-muon";

// Allows disabling the machine ID generation on Windows.
const char kDisableMachineId[] = "disable-machine-id";

// Allows disabling encryption on Windows for cookies, passwords, settings...
// WARNING! Use ONLY if your hard drive is encrypted or if you know
// what you are doing.
const char kDisableEncryptionWin[] = "disable-encryption-win";

// This enables smart tracking protection
const char kEnableSmartTrackingProtection[] =
    "enable-smart-tracking-protection";

}  // namespace switches
