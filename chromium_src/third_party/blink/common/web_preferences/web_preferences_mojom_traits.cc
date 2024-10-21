/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/public/common/web_preferences/web_preferences_mojom_traits.h"

#define WebPreferences WebPreferences_ChromiumImpl

#include "src/third_party/blink/common/web_preferences/web_preferences_mojom_traits.cc"

#undef WebPreferences

namespace mojo {

bool StructTraits<blink::mojom::WebPreferencesDataView,
                  blink::web_pref::WebPreferences>::
    Read(blink::mojom::WebPreferencesDataView data,
         blink::web_pref::WebPreferences* out) {
  if (!StructTraits<blink::mojom::WebPreferencesDataView,
                    blink::web_pref::WebPreferences_ChromiumImpl>::Read(data,
                                                                        out)) {
    return false;
  }
  out->force_cosmetic_filtering = data.force_cosmetic_filtering();
  out->page_in_reader_mode = data.page_in_reader_mode();
  return true;
}

}  // namespace mojo
