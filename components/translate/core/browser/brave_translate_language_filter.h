/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_TRANSLATE_CORE_BROWSER_BRAVE_TRANSLATE_LANGUAGE_FILTER_H_
#define BRAVE_COMPONENTS_TRANSLATE_CORE_BROWSER_BRAVE_TRANSLATE_LANGUAGE_FILTER_H_

#include <string>

namespace translate {

// Returns true if the source language |lang_code| is supported by Brave
// backend.
bool IsSourceLanguageCodeSupported(const std::string& lang_code);

// Returns true if the target language |lang_code| is supported by Brave
// backend.
bool IsTargetLanguageCodeSupported(const std::string& lang_code);

}  // namespace translate

#endif  // BRAVE_COMPONENTS_TRANSLATE_CORE_BROWSER_BRAVE_TRANSLATE_LANGUAGE_FILTER_H_
