/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_TRANSLATE_CORE_COMMON_BRAVE_TRANSLATE_CONSTANTS_H_
#define BRAVE_COMPONENTS_TRANSLATE_CORE_COMMON_BRAVE_TRANSLATE_CONSTANTS_H_

namespace translate {

inline constexpr char kBraveTranslateOrigin[] = "https://translate.brave.com";
inline constexpr char kBraveTranslateScriptURL[] =
    "https://translate.brave.com/static/v1/element.js";

// The used version of translate static resources (js/css files).
// Used in brave_translate.js as a replacement to /translate_static/ part in
// original script URLs.
inline constexpr char kBraveTranslateStaticPath[] = "/static/v1/";

}  // namespace translate

#endif  // BRAVE_COMPONENTS_TRANSLATE_CORE_COMMON_BRAVE_TRANSLATE_CONSTANTS_H_
