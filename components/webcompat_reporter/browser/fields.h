/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_WEBCOMPAT_REPORTER_BROWSER_FIELDS_H_
#define BRAVE_COMPONENTS_WEBCOMPAT_REPORTER_BROWSER_FIELDS_H_

#include "brave/components/brave_shields/core/common/brave_shields_panel.mojom.h"

namespace webcompat_reporter {

inline constexpr char kFPBlockSettingField[] = "fpBlockSetting";
inline constexpr char kAdBlockSettingField[] = "adBlockSetting";
inline constexpr char kAdBlockListsField[] = "adBlockLists";
inline constexpr char kAdBlockComponentsVersionField[] =
    "adBlockComponentsInfo";
inline constexpr char kShieldsEnabledField[] = "shieldsEnabled";
inline constexpr char kLanguagesField[] = "languages";
inline constexpr char kLanguageFarblingField[] = "languageFarblingEnabled";
inline constexpr char kBraveVPNEnabledField[] = "braveVPNEnabled";
inline constexpr char kChannelField[] = "channel";
inline constexpr char kVersionField[] = "version";
inline constexpr char kCookiePolicyField[] = "cookie_policy";
inline constexpr char kBlockScriptsField[] = "block_scripts";

inline constexpr char kSiteURLField[] = "url";
inline constexpr char kDomainField[] = "domain";
inline constexpr char kDetailsField[] = "additionalDetails";
inline constexpr char kContactField[] = "contactInfo";
inline constexpr char kApiKeyField[] = "api_key";

inline constexpr char kUISourceField[] = "ui_source";
inline constexpr char kIsErrorPage[] = "isErrorPage";

const char* GetAdBlockModeString(
    brave_shields::mojom::AdBlockMode ad_block_mode);
const char* GetFingerprintModeString(
    brave_shields::mojom::FingerprintMode fp_block_mode);

}  // namespace webcompat_reporter

#endif  // BRAVE_COMPONENTS_WEBCOMPAT_REPORTER_BROWSER_FIELDS_H_
