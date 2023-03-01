/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMMON_IMPORTER_IMPORTER_CONSTANTS_H_
#define BRAVE_COMMON_IMPORTER_IMPORTER_CONSTANTS_H_

#include "build/build_config.h"

// Prefs files that holds installed extension list.
constexpr char kChromeSecurePreferencesFile[] = "Secure Preferences";
constexpr char kChromePreferencesFile[] = "Preferences";

constexpr char kChromeExtensionsListPath[] = "extensions.settings";
constexpr char kChromeLocalStateFile[] = "Local State";

// Browser names section, the names mostly match the identifier that is used to
// identify the default browser, if you change these constants(or adding a new
// one) make sure that brave://welcome page will identify this browser as the
// default one
constexpr char kGoogleChromeBrowser[] = "Google Chrome";
constexpr char kGoogleChromeBrowserBeta[] = "Google Chrome Beta";
constexpr char kGoogleChromeBrowserDev[] = "Google Chrome Dev";
constexpr char kGoogleChromeBrowserCanary[] = "Google Chrome Canary";
constexpr char kChromiumBrowser[] = "Chromium";
constexpr char kMicrosoftEdgeBrowser[] = "Microsoft Edge";
constexpr char kVivaldiBrowser[] = "Vivaldi";
constexpr char kOperaBrowser[] = "Opera";
constexpr char kYandexBrowser[] = "Yandex";
constexpr char kWhaleBrowser[] = "NAVER Whale";
// End of browser names section

#endif  // BRAVE_COMMON_IMPORTER_IMPORTER_CONSTANTS_H_
