/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_SODA_CONSTANTS_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_SODA_CONSTANTS_H_

#include <components/soda/constants.h>  // IWYU pragma: export

#include <string>
#include <string_view>
#include <vector>

namespace speech {

// Whether Brave's on-device models transcribe `language`. The English model
// takes any English tag. The multilingual model takes only the tags it has a
// prompt for, ignoring case.
bool IsBraveOnDeviceSpeechLanguageSupported(std::string_view language);

// The SODA language pack names `available()` and `install()` hand Brave for
// the languages above. SODA maps a tag to the first pack sharing its language
// subtag, so "es-US" arrives as "es-ES".
std::vector<std::string> GetBraveOnDeviceSpeechSodaLanguageNames();

}  // namespace speech

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_SODA_CONSTANTS_H_
