// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/views/web_apps/web_app_views_utils.h"

#include "brave/browser/ui/brave_scheme_utils.h"

#define CreateOriginLabelFromStartUrl CreateOriginLabelFromStartUrl_ChromiumImpl
#include "src/chrome/browser/ui/views/web_apps/web_app_views_utils.cc"
#undef CreateOriginLabelFromStartUrl

namespace web_app {
std::unique_ptr<views::Label> CreateOriginLabelFromStartUrl(
    const GURL& start_url,
    bool is_primary_text) {
  std::unique_ptr<views::Label> origin_label =
      CreateOriginLabelFromStartUrl_ChromiumImpl(start_url, is_primary_text);
  CHECK(origin_label);
  std::u16string label_text = origin_label->GetText();
  brave_utils::ReplaceChromeToBraveScheme(label_text);
  origin_label->SetText(label_text);

  return origin_label;
}
}  // namespace web_app
