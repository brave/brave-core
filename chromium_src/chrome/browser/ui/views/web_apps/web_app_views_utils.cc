// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/views/web_apps/web_app_views_utils.h"

#define CreateOriginLabel CreateOriginLabel_ChromiumImpl
#include "src/chrome/browser/ui/views/web_apps/web_app_views_utils.cc"
#undef CreateOriginLabel

namespace web_app {

std::unique_ptr<views::Label> CreateOriginLabel(const url::Origin& origin,
                                                bool is_primary_text) {
  const url::Origin updated_origin = url::Origin::CreateFromNormalizedTuple(
      origin.scheme() == "chrome" ? "brave" : origin.scheme(), origin.host(),
      origin.port());
  return CreateOriginLabel_ChromiumImpl(updated_origin, is_primary_text);
}

}  // namespace web_app
