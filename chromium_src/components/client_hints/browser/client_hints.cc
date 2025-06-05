/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <iostream>

#include "components/client_hints/browser/client_hints.h"

#include "src/components/client_hints/browser/client_hints.cc"

namespace client_hints {

namespace {
constexpr char kGoogleChromeBrand[] = "Google Chrome";
}  // namespace

blink::UserAgentMetadata ClientHints::BraveGetUserAgentMetadata(
    bool showBraveBrand) {
  std::cout << "ClientHints::BraveGetUserAgentMetadata"
            << ", showBraveBrand=" << showBraveBrand << std::endl;
  auto metadata = GetUserAgentMetadata();
  if (!showBraveBrand) {
    for (auto& brand_version : metadata.brand_version_list) {
      std::cout << "brand=" << brand_version.brand << ", version=" << brand_version.version
                << std::endl;
      if (brand_version.brand == "Brave") {
        brand_version.brand = kGoogleChromeBrand;
        break;
      }
    }
  }
  return metadata;
}

}  // namespace client_hints
