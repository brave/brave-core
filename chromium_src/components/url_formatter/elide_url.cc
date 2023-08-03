/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#define FormatOriginForSecurityDisplay \
  FormatOriginForSecurityDisplay_ChromiumImpl
#include "src/components/url_formatter/elide_url.cc"
#undef FormatOriginForSecurityDisplay

namespace url_formatter {

std::u16string FormatOriginForSecurityDisplay(
    const url::Origin& origin,
    const SchemeDisplay scheme_display) {
  auto display_origin =
      FormatOriginForSecurityDisplay_ChromiumImpl(origin, scheme_display);
  if (origin.scheme() == "chrome") {
    base::ReplaceFirstSubstringAfterOffset(&display_origin, 0, u"chrome",
                                           u"brave");
  }

  return display_origin;
}

}  // namespace url_formatter
