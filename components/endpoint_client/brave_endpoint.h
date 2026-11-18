/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_ENDPOINT_CLIENT_BRAVE_ENDPOINT_H_
#define BRAVE_COMPONENTS_ENDPOINT_CLIENT_BRAVE_ENDPOINT_H_

#include <string_view>

#include "brave/components/endpoint_client/is_request.h"
#include "brave/components/endpoint_client/is_response.h"
#include "brave/components/endpoint_client/static_string.h"
#include "url/gurl.h"

namespace endpoint_client {

namespace internal {

GURL MakeBraveServiceURL(std::string_view prefix, std::string_view path);

}  // namespace internal

template <internal::StaticString Prefix,
          internal::StaticString Path,
          internal::IsRequest RequestT,
          internal::IsResponse ResponseT>
struct BraveEndpoint {
  using Request = RequestT;
  using Response = ResponseT;

  static GURL URL() {
    return internal::MakeBraveServiceURL(Prefix.value, Path.value);
  }
};

}  // namespace endpoint_client

#endif  // BRAVE_COMPONENTS_ENDPOINT_CLIENT_BRAVE_ENDPOINT_H_
