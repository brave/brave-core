/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/rust/parse_and_sanitize_html_util.h"

#include "brave/components/brave_ads/rust/lib.rs.h"

namespace brave_ads::rust {

std::string ParseAndSanitizeOgTagAttribute(const std::string& html) {
  return static_cast<std::string>(parse_and_sanitize_og_tag_attribute(html));
}

}  // namespace brave_ads::rust
