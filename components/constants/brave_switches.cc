/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/constants/brave_switches.h"

namespace switches {

// Allows disabling the Brave extension.
// This is commonly used for loading the extension manually to debug things
// in debug mode with auto-reloading.
const char kDisableBraveExtension[] = "disable-brave-extension";

// Allows disabling the Brave Rewards extension.
const char kDisableBraveRewardsExtension[] = "disable-brave-rewards-extension";

// This switch disables update module(Sparkle).
const char kDisableBraveUpdate[] = "disable-brave-update";

// Allows disabling the WebTorrent extension.
const char kDisableWebTorrentExtension[] = "disable-webtorrent-extension";

// Allows disabling the Wayback Machine extension.
const char kDisableBraveWaybackMachineExtension[] =
    "disable-brave-wayback-machine-extension";

// Specifies overriding the built-in dark mode setting.
// Valid values are: "dark" | "light".
const char kDarkMode[] = "dark-mode";

// Allows disabling the machine ID generation on Windows.
const char kDisableMachineId[] = "disable-machine-id";

// Allows disabling encryption on Windows for cookies, passwords, settings...
// WARNING! Use ONLY if your hard drive is encrypted or if you know
// what you are doing.
const char kDisableEncryptionWin[] = "disable-encryption-win";

// Use custom update interval in sec
const char kComponentUpdateIntervalInSec[] = "component-update-interval-in-sec";

// Disables DOH using a runtime flag mainly for network audit
const char kDisableDnsOverHttps[] = "disable-doh";

// Override update feed url. Only valid on macOS.
const char kUpdateFeedURL[] = "update-feed-url";

// Starts Brave in Tor mode.
const char kTor[] = "tor";
}  // namespace switches
