/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMMON_IMPORTER_IMPORTER_CONSTANTS_H_
#define BRAVE_COMMON_IMPORTER_IMPORTER_CONSTANTS_H_

#include "build/build_config.h"

// Prefs files that holds installed extension list.
inline constexpr char kChromeSecurePreferencesFile[] = "Secure Preferences";
inline constexpr char kChromePreferencesFile[] = "Preferences";

inline constexpr char kChromeExtensionsListPath[] = "extensions.settings";
inline constexpr char kChromeLocalStateFile[] = "Local State";

// Browser names section, the names mostly match the identifier that is used to
// identify the default browser, if you change these constants(or adding a new
// one) make sure that brave://welcome page will identify this browser as the
// default one
inline constexpr char kGoogleChromeBrowser[] = "Google Chrome";
inline constexpr char kGoogleChromeBrowserBeta[] = "Google Chrome Beta";
inline constexpr char kGoogleChromeBrowserDev[] = "Google Chrome Dev";
inline constexpr char kGoogleChromeBrowserCanary[] = "Google Chrome Canary";
inline constexpr char kChromiumBrowser[] = "Chromium";
inline constexpr char kMicrosoftEdgeBrowser[] = "Microsoft Edge";
inline constexpr char kVivaldiBrowser[] = "Vivaldi";
inline constexpr char kOperaBrowser[] = "Opera";
inline constexpr char kYandexBrowser[] = "Yandex";
inline constexpr char kWhaleBrowser[] = "NAVER Whale";
// End of browser names section

#endif  // BRAVE_COMMON_IMPORTER_IMPORTER_CONSTANTS_H_
