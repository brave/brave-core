/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_PUBLIC_COMMON_WEB_PREFERENCES_WEB_PREFERENCES_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_PUBLIC_COMMON_WEB_PREFERENCES_WEB_PREFERENCES_H_

#define WebPreferences WebPreferences_ChromiumImpl

#include "src/third_party/blink/public/common/web_preferences/web_preferences.h"  // IWYU pragma: export

#undef WebPreferences

namespace blink {

namespace web_pref {

struct BLINK_COMMON_EXPORT WebPreferences : public WebPreferences_ChromiumImpl {
  using WebPreferences_ChromiumImpl::WebPreferences_ChromiumImpl;

  WebPreferences(const WebPreferences& other);
  WebPreferences(WebPreferences&& other);
  ~WebPreferences();
  WebPreferences& operator=(const WebPreferences& other);
  WebPreferences& operator=(WebPreferences&& other);

  bool force_cosmetic_filtering = false;
  bool page_in_reader_mode = false;
};

}  // namespace web_pref

}  // namespace blink

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_PUBLIC_COMMON_WEB_PREFERENCES_WEB_PREFERENCES_H_
