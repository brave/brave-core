/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_CONSTANTS_URL_CONSTANTS_H_
#define BRAVE_COMPONENTS_CONSTANTS_URL_CONSTANTS_H_

extern const char kChromeExtensionScheme[];
extern const char kBraveUIScheme[];
extern const char kMagnetScheme[];
extern const char kBinanceScheme[];
extern const char kGeminiScheme[];
extern const char kFTXScheme[];
extern const char kWidevineTOS[];
extern const char kRewardsUpholdSupport[];
extern const char kP3ALearnMoreURL[];
extern const char kP3ASettingsLink[];
extern const char kImportDataHelpURL[];
extern const char kCryptoWalletsLearnMoreURL[];
extern const char kPermissionPromptLearnMoreUrl[];
extern const char kSpeedreaderLearnMoreUrl[];
extern const char kWebDiscoveryLearnMoreUrl[];
extern const char kBraveSearchUrl[];
extern const char kWidevineLearnMoreUrl[];

// This is introduced to replace |kDownloadChromeUrl| in
// outdated_upgrade_bubble_view.cc"
// |kDownloadChromeUrl| couldn't be replaced with char array because array
// should be initialized with initialize list or string literal.
// So, this macro is used.
#define kDownloadBraveUrl "https://www.brave.com/download"

#endif  // BRAVE_COMPONENTS_CONSTANTS_URL_CONSTANTS_H_
