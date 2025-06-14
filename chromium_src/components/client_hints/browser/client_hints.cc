/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/client_hints/browser/client_hints.h"

#include "src/components/client_hints/browser/client_hints.cc"

#include "brave/components/brave_user_agent/browser/brave_user_agent_exceptions.h"

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
    GURL top_url) {
  auto* brave_user_agent_exceptions = brave_user_agent::BraveUserAgentExceptions::GetInstance();
  bool canShowBrave = brave_user_agent_exceptions->CanShowBrave(top_url);
  auto metadata = GetUserAgentMetadata();
  if (!canShowBrave) {
    removeBrandFromVersionList(metadata.brand_version_list);
    removeBrandFromVersionList(metadata.brand_full_version_list);
  }
  return metadata;
}

}  // namespace client_hints
