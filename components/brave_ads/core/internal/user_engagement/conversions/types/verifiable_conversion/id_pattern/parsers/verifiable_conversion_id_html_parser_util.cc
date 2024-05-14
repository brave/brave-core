/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/conversions/types/verifiable_conversion/id_pattern/parsers/verifiable_conversion_id_html_parser_util.h"

#include <string_view>

#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/resource/conversion_resource_id_pattern_info.h"
#include "third_party/re2/src/re2/re2.h"

namespace brave_ads {

std::optional<std::string> MaybeParseVerifableConversionIdFromHtml(
    const std::string& html,
    const ConversionResourceIdPatternInfo& resource_id_pattern) {
  std::string_view html_string_piece(html);
  const RE2 r(resource_id_pattern.id_pattern);
  std::string verifiable_conversion_id;

  if (!RE2::FindAndConsume(&html_string_piece, r, &verifiable_conversion_id)) {
    BLOG(1, "Failed to parse verifiable conversion id for "
                << resource_id_pattern.id_pattern << " resource id pattern");
    return std::nullopt;
  }

  return verifiable_conversion_id;
}

}  // namespace brave_ads
