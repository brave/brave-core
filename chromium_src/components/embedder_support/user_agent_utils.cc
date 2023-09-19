/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/embedder_support/user_agent_utils.h"

namespace {

constexpr char kBraveBrandNameForCHUA[] = "Brave";

}  // namespace

// Chromium uses `version_info::GetProductName()` to get the browser's "brand"
// name, but on MacOS we use different names for different channels (adding Beta
// or Nightly, for example). In the UA client hint, though, we want a consistent
// name regardless of the channel, so we just hard-code it. Note, that we use
// IDS_PRODUCT_NAME from app/chromium_strings.grd (brave_strings.grd) in
// constructing the UA in brave/browser/brave_content_browser_client.cc, but we
// can't use it here in the //components.
#define BRAVE_GET_USER_AGENT_BRAND_LIST brand = kBraveBrandNameForCHUA;

#include "src/components/embedder_support/user_agent_utils.cc"
#undef BRAVE_GET_USER_AGENT_BRAND_LIST
