/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "../../../../chrome/browser/flag_descriptions.cc"

namespace flag_descriptions {
const char kUseDevUpdaterUrlName[] =
    "Use dev updater url";
const char kUseDevUpdaterUrlDescription[] =
    "Use the dev url for the component updater. "
    "This is for internal testing only.";
const char kBraveNTPBrandedWallpaperName[] =
    "New Tab Page Branded Wallpapers";
const char kBraveNTPBrandedWallpaperDescription[] =
    "Allow New Tab Page Branded Wallpapers and user preference.";
const char kBraveNTPBrandedWallpaperDemoName[] =
    "New Tab Page Demo Branded Wallpaper";
const char kBraveNTPBrandedWallpaperDemoDescription[] =
    "Force dummy data for the Branded Wallpaper New Tab Page Experience. "
    "View rate and user opt-in conditionals will still be followed to decide "
    "when to display the Branded Wallpaper.";
const char kBraveAdblockCosmeticFilteringName[] = "Enable cosmetic filtering";
const char kBraveAdblockCosmeticFilteringDescription[] =
    "Enable support for cosmetic filtering";
const char kBraveSidebarName[] = "Enable Sidebar";
// TODO(simon): Use more better description.
const char kBraveSidebarDescription[] = "Enable Sidebar";
const char kBraveSpeedreaderName[] = "Enable SpeedReader";
const char kBraveSpeedreaderDescription[] =
    "Enables faster loading of simplified article-style web pages.";
const char kBraveSyncName[] = "Enable Brave Sync v2";
const char kBraveSyncDescription[] =
    "Brave Sync v2 integrates with chromium sync engine with Brave specific "
    "authentication flow and enforce client side encryption";
const char kBraveIpfsName[] = "Enable IPFS";
const char kBraveIpfsDescription[] =
    "Enable native support of IPFS.";
const char kBraveSuperReferralName[] = "Enable Brave Super Referral";
const char kBraveSuperReferralDescription[] =
    "Use custom theme for Brave Super Referral";
const char kBraveEphemeralStorageName[] = "Enable Ephemeral Storage";
const char kBraveEphemeralStorageDescription[] =
    "Use ephemeral storage for third-party frames";
}  // namespace flag_descriptions
