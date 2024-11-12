/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/public/common/web_preferences/web_preferences.h"
#include "third_party/blink/public/mojom/webpreferences/web_preferences.mojom.h"

#define WebPreferences WebPreferences_ChromiumImpl

#include "src/third_party/blink/common/web_preferences/web_preferences.cc"

#undef WebPreferences

namespace blink::web_pref {

WebPreferences::WebPreferences(const WebPreferences& other) = default;
WebPreferences::WebPreferences(WebPreferences&& other) = default;
WebPreferences::~WebPreferences() = default;
WebPreferences& WebPreferences::operator=(const WebPreferences& other) =
    default;
WebPreferences& WebPreferences::operator=(WebPreferences&& other) = default;

}  // namespace blink::web_pref
