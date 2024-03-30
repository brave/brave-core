/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/conversions/types/verifiable_conversion/id_pattern/verifiable_conversion_id_pattern_util.h"

#include "base/check.h"
#include "brave/components/brave_ads/core/internal/common/url/url_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/resource/conversion_resource_id_pattern_search_in_types.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/types/verifiable_conversion/id_pattern/parsers/verifiable_conversion_id_html_meta_tag_parser_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/types/verifiable_conversion/id_pattern/parsers/verifiable_conversion_id_html_parser_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/types/verifiable_conversion/id_pattern/parsers/verifiable_conversion_id_url_redirects_parser_util.h"
#include "url/gurl.h"

namespace brave_ads {

namespace {

std::optional<ConversionResourceIdPatternInfo>
FindMatchingConversionResourceIdPattern(
    const ConversionResourceIdPatternMap& resource_id_patterns,
    const std::vector<GURL>& redirect_chain) {
  CHECK(!redirect_chain.empty());

  if (resource_id_patterns.empty()) {
    return std::nullopt;
  }

  for (const auto& [url_pattern, resource_id_pattern] : resource_id_patterns) {
    if (MatchUrlPattern(redirect_chain, url_pattern)) {
      return resource_id_pattern;
    }
  }

  return std::nullopt;
}

std::optional<std::string>
MaybeParseResourceIdPatternSearchInTypeVerifiableConversionId(
    const std::vector<GURL>& redirect_chain,
    const std::string& html,
    const ConversionResourceIdPatternInfo& resource_id_pattern) {
  switch (resource_id_pattern.search_in_type) {
    case ConversionResourceIdPatternSearchInType::kUrlRedirect: {
      return MaybeParseVerifableConversionIdFromUrlRedirects(
          redirect_chain, resource_id_pattern);
    }

    case ConversionResourceIdPatternSearchInType::kHtml: {
      return MaybeParseVerifableConversionIdFromHtml(html, resource_id_pattern);
    }

    default: {
      return std::nullopt;
    }
  }
}

std::optional<std::string> MaybeParseDefaultVerifiableConversionId(
    const std::string& html) {
  return MaybeParseVerifableConversionIdFromHtmlMetaTag(html);
}

}  // namespace

std::optional<std::string> MaybeParseVerifiableConversionId(
    const std::vector<GURL>& redirect_chain,
    const std::string& html,
    const ConversionResourceIdPatternMap& resource_id_patterns) {
  const std::optional<ConversionResourceIdPatternInfo> resource_id_pattern =
      FindMatchingConversionResourceIdPattern(resource_id_patterns,
                                              redirect_chain);
  if (!resource_id_pattern) {
    return MaybeParseDefaultVerifiableConversionId(html);
  }

  if (const std::optional<std::string> verifiable_conversion_id =
          MaybeParseResourceIdPatternSearchInTypeVerifiableConversionId(
              redirect_chain, html, *resource_id_pattern)) {
    return verifiable_conversion_id;
  }

  return MaybeParseDefaultVerifiableConversionId(html);
}

}  // namespace brave_ads
