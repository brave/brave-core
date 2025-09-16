/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/browser/service/network_client_util.h"

#include <string>

#include "base/check.h"
#include "base/notreached.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "net/http/http_request_headers.h"

namespace brave_ads {

std::string ToString(mojom::UrlRequestMethodType value) {
  CHECK(mojom::IsKnownEnumValue(value));

  switch (value) {
    case mojom::UrlRequestMethodType::kGet: {
      return net::HttpRequestHeaders::kGetMethod;
    }

    case mojom::UrlRequestMethodType::kPost: {
      return net::HttpRequestHeaders::kPostMethod;
    }

    case mojom::UrlRequestMethodType::kPut: {
      return net::HttpRequestHeaders::kPutMethod;
    }
  }

  NOTREACHED() << "Unexpected value for mojom::UrlRequestMethodType: " << value;
}

}  // namespace brave_ads
