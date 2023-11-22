/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_url_response_headers_util.h"

#include "base/check_op.h"
#include "base/strings/string_split.h"

namespace brave_ads {

base::flat_map<std::string, std::string> ToUrlResponseHeaders(
    const std::vector<std::string>& headers) {
  base::flat_map<std::string, std::string> response_headers;

  for (const auto& header : headers) {
    const std::vector<std::string> components = base::SplitString(
        header, ":", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
    CHECK_EQ(2U, components.size()) << "Invalid header: " << header;

    const std::string& name = components.at(0);
    const std::string& value = components.at(1);

    response_headers[name] = value;
  }

  return response_headers;
}

}  // namespace brave_ads
