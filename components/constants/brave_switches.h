/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_CONSTANTS_BRAVE_SWITCHES_H_
#define BRAVE_COMPONENTS_CONSTANTS_BRAVE_SWITCHES_H_

namespace switches {

// All switches in alphabetical order. The switches should be documented
// alongside the definition of their values in the .cc file.

// Use custom update interval in sec
inline constexpr char kComponentUpdateIntervalInSec[] =
    "component-update-interval-in-sec";

// Specifies overriding the built-in dark mode setting.
// Valid values are: "dark" | "light".
inline constexpr char kDarkMode[] = "dark-mode";

// Allows disabling the Brave extension.
// This is commonly used for loading the extension manually to debug things
// in debug mode with auto-reloading.
inline constexpr char kDisableBraveExtension[] = "disable-brave-extension";

// This switch disables update module(Sparkle).
inline constexpr char kDisableBraveUpdate[] = "disable-brave-update";

// Allows disabling the WebTorrent extension.
inline constexpr char kDisableWebTorrentExtension[] =
    "disable-webtorrent-extension";

// Allows disabling the Wayback Machine extension.
inline constexpr char kDisableBraveWaybackMachineExtension[] =
    "disable-brave-wayback-machine-extension";

// Allows disabling encryption on Windows for cookies, passwords, settings...
// WARNING! Use ONLY if your hard drive is encrypted or if you know
// what you are doing.
inline constexpr char kDisableEncryptionWin[] = "disable-encryption-win";

// Allows disabling the machine ID generation on Windows.
inline constexpr char kDisableMachineId[] = "disable-machine-id";

// Starts Brave in Tor mode.
inline constexpr char kTor[] = "tor";

// Override update feed url. Only valid on macOS.
inline constexpr char kUpdateFeedURL[] = "update-feed-url";

// Don't set kShowAlways for non-stable channel.
// It's useful to test SidebarShowAlwaysOnStable w/o griffin.
inline constexpr char kDontShowSidebarOnNonStable[] =
    "dont-show-on-sidebar-non-stable";

}  // namespace switches

#endif  // BRAVE_COMPONENTS_CONSTANTS_BRAVE_SWITCHES_H_
