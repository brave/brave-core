/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/conversions/types/verifiable_conversion/verifiable_conversion_id_pattern/parsers/verifiable_conversion_id_url_redirects_parser_util.h"

#include "base/ranges/algorithm.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/url/url_util.h"
#include "brave/components/brave_ads/core/internal/conversions/resource/conversion_resource_id_pattern_info.h"
#include "third_party/re2/src/re2/re2.h"
#include "url/gurl.h"

namespace brave_ads {

absl::optional<std::string> MaybeParseVerifableConversionIdFromUrlRedirects(
    const std::vector<GURL>& redirect_chain,
    const ConversionResourceIdPatternInfo& resource_id_pattern) {
  const auto iter = base::ranges::find_if(
      redirect_chain, [&resource_id_pattern](const GURL& url) {
        return MatchUrlPattern(url, resource_id_pattern.url_pattern);
      });

  if (iter == redirect_chain.cend()) {
    // Resource id pattern was not found in the URL redirect chain.
    return absl::nullopt;
  }

  re2::StringPiece url_string_piece(iter->spec());
  const RE2 r(resource_id_pattern.id_pattern);
  std::string verifiable_conversion_id;

  if (!RE2::FindAndConsume(&url_string_piece, r, &verifiable_conversion_id)) {
    BLOG(1, "Failed to parse verifiable conversion id for "
                << resource_id_pattern.id_pattern << " resource id pattern");
    return absl::nullopt;
  }

  return verifiable_conversion_id;
}

}  // namespace brave_ads
