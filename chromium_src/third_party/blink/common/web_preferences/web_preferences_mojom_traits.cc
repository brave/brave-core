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
  out->hide_media_src_api = data.hide_media_src_api();
  out->should_detect_media_files = data.should_detect_media_files();
  out->allow_to_run_script_on_main_world =
      data.allow_to_run_script_on_main_world();

  mojo::MapDataView<mojo::StringDataView, mojo::StringDataView> data_view;
  data.GetUrlAndMediaDetectionScriptsDataView(&data_view);

  for (size_t i = 0; i < data_view.size(); ++i) {
    std::string key;
    std::string value;
    if (!data_view.keys().Read(i, &key) ||
        !data_view.values().Read(i, &value)) {
      continue;
    }
    out->url_and_media_detection_scripts.insert({key, value});
  }

  return true;
}

}  // namespace mojo
