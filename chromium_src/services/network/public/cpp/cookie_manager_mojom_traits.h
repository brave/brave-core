/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_SERVICES_NETWORK_PUBLIC_CPP_COOKIE_MANAGER_MOJOM_TRAITS_H_
#define BRAVE_CHROMIUM_SRC_SERVICES_NETWORK_PUBLIC_CPP_COOKIE_MANAGER_MOJOM_TRAITS_H_

#include "net/cookies/canonical_cookie.h"
#include "net/cookies/cookie_access_result.h"
#include "net/cookies/cookie_change_dispatcher.h"
#include "net/cookies/cookie_options.h"
#include "net/cookies/cookie_partition_key_collection.h"
#include "services/network/public/mojom/cookie_manager.mojom.h"

#define CookieOptions CookieOptions_ChromiumImpl

#include "src/services/network/public/cpp/cookie_manager_mojom_traits.h"

#undef CookieOptions

namespace mojo {

template <>
struct StructTraits<network::mojom::CookieOptionsDataView, net::CookieOptions>
    : public StructTraits<network::mojom::CookieOptionsDataView,
                          net::CookieOptions_ChromiumImpl> {
  static const net::SiteForCookies& site_for_cookies(
      const net::CookieOptions& o) {
    return o.site_for_cookies();
  }
  static const absl::optional<url::Origin>& top_frame_origin(
      const net::CookieOptions& o) {
    return o.top_frame_origin();
  }
  static bool should_use_ephemeral_storage(const net::CookieOptions& o) {
    return o.should_use_ephemeral_storage();
  }

  static bool Read(network::mojom::CookieOptionsDataView mojo_options,
                   net::CookieOptions* cookie_options);
};

}  // namespace mojo

#endif  // BRAVE_CHROMIUM_SRC_SERVICES_NETWORK_PUBLIC_CPP_COOKIE_MANAGER_MOJOM_TRAITS_H_
