/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/client_hints/browser/client_hints.h"

#include "src/components/client_hints/browser/client_hints.cc"

namespace client_hints {

namespace {

constexpr char kGoogleChromeBrand[] = "Google Chrome";

void removeBrandFromVersionList(blink::UserAgentBrandList& brand_version_list) {
  for (auto& brand_version : brand_version_list) {
    if (brand_version.brand == "Brave") {
      brand_version.brand = kGoogleChromeBrand;
      break;
    }
  }
}

}  // namespace

blink::UserAgentMetadata ClientHints::BraveGetUserAgentMetadata(
    bool showBraveBrand) {
  auto metadata = GetUserAgentMetadata();
  if (!showBraveBrand) {
    removeBrandFromVersionList(metadata.brand_version_list);
    removeBrandFromVersionList(metadata.brand_full_version_list);
  }
  return metadata;
}

}  // namespace client_hints
