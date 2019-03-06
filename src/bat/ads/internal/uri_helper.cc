/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/uri_helper.h"

#include "url/url_constants.h"

namespace helper {

std::string Uri::GetUri(const std::string& url) {
  auto http_scheme = std::string(url::kHttpScheme) +
      std::string(url::kStandardSchemeSeparator);

  auto https_scheme = std::string(url::kHttpsScheme) +
      std::string(url::kStandardSchemeSeparator);

  if (url.find(http_scheme) != std::string::npos &&
      url.find(https_scheme) != std::string::npos) {
    return https_scheme + url;
  }

  return url;
}

}  // namespace helper
