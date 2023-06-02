/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/sanitize/sanitize_util.h"

#include "brave/components/text_sanitize/rs/src/lib.rs.h"

namespace brave_ads {

std::string SanitizeHtmlContent(const std::string& text) {
  return static_cast<std::string>(
      text_sanitize::strip_non_alphanumeric_or_ascii_characters(text));
}

}  // namespace brave_ads
