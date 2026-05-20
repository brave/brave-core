// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/query_filter/browser/utils.h"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "base/containers/fixed_flat_map.h"
#include "base/containers/fixed_flat_set.h"
#include "base/containers/span.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "brave/components/query_filter/browser/query_filter_data.h"
#include "brave/components/query_filter/common/features.h"
#include "brave/components/query_filter/common/internal/schema_utils.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "third_party/re2/src/re2/re2.h"

namespace {

// The "key" is the parameter to remove from the url. This would only get
// removed if the "value" was NOT found in the url.
static constexpr auto kConditionalQueryStringTrackers =
    base::MakeFixedFlatMap<std::string_view, std::string_view>({
        // https://github.com/brave/brave-browser/issues/44341
        {"ck_subscriber_id", "/unsubscribe"},
        // https://github.com/brave/brave-browser/issues/30731
        {"h_sid", "/email/"},
        {"h_slt", "/email/"},
        // https://github.com/brave/brave-browser/issues/9018
        {"mkt_tok", "([uU]nsubscribe|emailWebview)"},
    });

// URLs with these hostnames will not be modified by the query filter.
// These are exact match comparisons. Sub-domains are not
// automatically included.
static constexpr auto kExemptedHostnames =
    base::MakeFixedFlatSet<std::string_view>(
        base::sorted_unique,
        {
            // https://github.com/brave/brave-browser/issues/41134
            "urldefense.com",
        });

// Remove tracking query parameters from a GURL, leaving all
// other parts untouched.
std::optional<std::string> StripQueryParameter(std::string_view query,
                                               std::string_view spec) {
  // We are using custom query string parsing code here. See
  // https://github.com/brave/brave-core/pull/13726#discussion_r897712350
  // for more information on why this approach was selected.
  //
  // Split query string by ampersands, remove tracking parameters,
  // then join the remaining query parameters, untouched, back into
  // a single query string.
  const std::vector<std::string_view> input_kv_strings =
      SplitStringPiece(query, "&", base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);
  std::vector<std::string_view> output_kv_strings;
  int disallowed_count = 0;

  // Set of all the params which are blocked for |spec| from our rules.
  const auto& rules = query_filter::QueryFilterData::GetInstance()->rules();
  const base::flat_set<std::string> blocked_params_set =
      query_filter::GetBlocklistedParamsForSpec(base::span(rules), spec);

  for (const auto& kv_string : input_kv_strings) {
    const std::vector<std::string_view> pieces = SplitStringPiece(
        kv_string, "=", base::KEEP_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
    const std::string_view key = pieces.empty() ? "" : pieces[0];
    if (pieces.size() >= 2 &&
        (blocked_params_set.contains(key) ||
         (kConditionalQueryStringTrackers.count(key) == 1 &&
          !re2::RE2::PartialMatch(
              spec, kConditionalQueryStringTrackers.at(key).data())))) {
      ++disallowed_count;
    } else {
      output_kv_strings.push_back(kv_string);
    }
  }
  if (disallowed_count > 0) {
    return base::JoinString(output_kv_strings, "&");
  }
  return std::nullopt;
}

}  // namespace

namespace query_filter {

std::optional<GURL> ApplyQueryFilter(const GURL& original_url) {
  std::string_view query = original_url.query();
  const std::string& spec = original_url.spec();
  const auto clean_query_value = StripQueryParameter(query, spec);
  if (!clean_query_value.has_value()) {
    return std::nullopt;
  }
  const auto& clean_query = clean_query_value.value();
  if (clean_query.length() < query.length()) {
    GURL::Replacements replacements;
    if (clean_query.empty()) {
      replacements.ClearQuery();
    } else {
      replacements.SetQueryStr(clean_query);
    }
    return original_url.ReplaceComponents(replacements);
  }
  return std::nullopt;
}

std::optional<GURL> MaybeApplyQueryStringFilter(
    const GURL& initiator_url,
    const GURL& redirect_source_url,
    const GURL& request_url,
    const std::string& request_method,
    const bool internal_redirect) {
  if (!request_url.has_query()) {
    // Optimization:
    // If there are no query params then we have nothing to strip.
    return std::nullopt;
  }
  if (request_method != "GET") {
    return std::nullopt;
  }

  if (request_url.has_host() &&
      kExemptedHostnames.count(request_url.host()) == 1) {
    return std::nullopt;
  }

  if (redirect_source_url.is_valid()) {
    if (internal_redirect) {
      // Ignore internal redirects since we trigger them.
      return std::nullopt;
    }

    if (net::registry_controlled_domains::SameDomainOrHost(
            redirect_source_url, request_url,
            net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES)) {
      // Same-site redirects are exempted.
      return std::nullopt;
    }
  } else if (initiator_url.is_valid() &&
             net::registry_controlled_domains::SameDomainOrHost(
                 initiator_url, request_url,
                 net::registry_controlled_domains::
                     INCLUDE_PRIVATE_REGISTRIES)) {
    // Same-site requests are exempted.
    return std::nullopt;
  }

  if (!base::FeatureList::IsEnabled(
          query_filter::features::kQueryFilterComponent)) {
    return std::nullopt;
  }

  return ApplyQueryFilter(request_url);
}
}  // namespace query_filter
