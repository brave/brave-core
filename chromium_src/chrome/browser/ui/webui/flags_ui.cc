/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/grit/brave_components_resources.h"
#include "content/public/browser/web_ui_data_source.h"

namespace {

constexpr char kBraveFlagsOverridesJS[] = "brave_flags_overrides.js";

void AddBraveFlagsUIHTMLSource(content::WebUIDataSource* source) {
  source->AddResourcePath(kBraveFlagsOverridesJS,
                          IDR_FLAGS_UI_BRAVE_FLAGS_OVERRIDES_JS);
}

}  // namespace

#define ADD_BRAVE_FLAGS_UI_HTML_SOURCE \
  AddBraveFlagsUIHTMLSource(source);

#include "../../../../../../chrome/browser/ui/webui/flags_ui.cc"  // NOLINT

#undef ADD_BRAVE_FLAGS_UI_HTML_SOURCE
