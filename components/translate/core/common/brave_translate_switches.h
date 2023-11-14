/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_TRANSLATE_CORE_COMMON_BRAVE_TRANSLATE_SWITCHES_H_
#define BRAVE_COMPONENTS_TRANSLATE_CORE_COMMON_BRAVE_TRANSLATE_SWITCHES_H_

namespace translate {
namespace switches {

// A test switch to disable the redirection for the translation requests to
// translate.brave.com.
inline constexpr char kBraveTranslateUseGoogleEndpoint[] =
    "use-google-translate-endpoint";

}  // namespace switches
}  // namespace translate

#endif  // BRAVE_COMPONENTS_TRANSLATE_CORE_COMMON_BRAVE_TRANSLATE_SWITCHES_H_
