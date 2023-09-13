/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/url_formatter/elide_url.h"

#define FormatOriginForSecurityDisplay \
  FormatOriginForSecurityDisplay_ChromiumImpl

#include "src/components/url_formatter/elide_url.cc"
#undef FormatOriginForSecurityDisplay

namespace url_formatter {

std::u16string FormatOriginForSecurityDisplay(
    const url::Origin& origin,
    const SchemeDisplay scheme_display) {
  const url::Origin updated_origin = url::Origin::CreateFromNormalizedTuple(
      origin.scheme() == "chrome" ? "brave" : origin.scheme(), origin.host(),
      origin.port());
  return FormatOriginForSecurityDisplay_ChromiumImpl(updated_origin,
                                                     scheme_display);
}

}  // namespace url_formatter
