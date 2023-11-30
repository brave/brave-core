/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <optional>

#include "services/network/public/cpp/cookie_manager_mojom_traits.h"
#include "services/network/public/mojom/cookie_manager.mojom.h"

#define CookieOptions CookieOptions_ChromiumImpl

#include "src/services/network/public/cpp/cookie_manager_mojom_traits.cc"

#undef CookieOptions

namespace mojo {

bool StructTraits<network::mojom::CookieOptionsDataView, net::CookieOptions>::
    Read(network::mojom::CookieOptionsDataView data, net::CookieOptions* out) {
  if (!StructTraits<network::mojom::CookieOptionsDataView,
                    net::CookieOptions_ChromiumImpl>::Read(data, out)) {
    return false;
  }

  net::SiteForCookies site_for_cookies;
  if (!data.ReadSiteForCookies(&site_for_cookies))
    return false;
  out->set_site_for_cookies(site_for_cookies);

  std::optional<url::Origin> top_frame_origin;
  if (!data.ReadTopFrameOrigin(&top_frame_origin))
    return false;
  out->set_top_frame_origin(top_frame_origin);

  out->set_should_use_ephemeral_storage(data.should_use_ephemeral_storage());

  return true;
}

}  // namespace mojo
