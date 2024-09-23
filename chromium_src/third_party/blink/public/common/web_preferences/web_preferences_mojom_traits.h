/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_PUBLIC_COMMON_WEB_PREFERENCES_WEB_PREFERENCES_MOJOM_TRAITS_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_PUBLIC_COMMON_WEB_PREFERENCES_WEB_PREFERENCES_MOJOM_TRAITS_H_

#include "third_party/blink/public/common/web_preferences/web_preferences.h"
#include "third_party/blink/public/mojom/webpreferences/web_preferences.mojom.h"

#define WebPreferences WebPreferences_ChromiumImpl

#include "src/third_party/blink/public/common/web_preferences/web_preferences_mojom_traits.h"  // IWYU pragma: export

#undef WebPreferences

namespace mojo {

template <>
struct BLINK_COMMON_EXPORT StructTraits<blink::mojom::WebPreferencesDataView,
                                        blink::web_pref::WebPreferences>
    : public StructTraits<blink::mojom::WebPreferencesDataView,
                          blink::web_pref::WebPreferences_ChromiumImpl> {
  static bool force_cosmetic_filtering(
      const blink::web_pref::WebPreferences& r) {
    return r.force_cosmetic_filtering;
  }

  static bool page_in_reader_mode(const blink::web_pref::WebPreferences& r) {
    return r.page_in_reader_mode;
  }

  static bool Read(blink::mojom::WebPreferencesDataView r,
                   blink::web_pref::WebPreferences* out);
};

}  // namespace mojo

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_PUBLIC_COMMON_WEB_PREFERENCES_WEB_PREFERENCES_MOJOM_TRAITS_H_
