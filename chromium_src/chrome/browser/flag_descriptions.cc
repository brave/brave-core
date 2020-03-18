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
const char kBraveSyncName[] = "Enable Brave Sync";
const char kBraveSyncDescription[] = "Brave Sync is disabled by default";
const char kBraveCustomHomepageName[] = "Enable Brave Custom Homepage";
const char kBraveCustomHomepageDescription[] = "Show Brave Custom Homepage";
}  // namespace flag_descriptions
