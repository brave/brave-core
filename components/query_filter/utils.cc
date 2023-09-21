// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/query_filter/utils.h"

#include <string>
#include <string_view>
#include <vector>

#include "base/containers/fixed_flat_map.h"
#include "base/containers/fixed_flat_set.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "third_party/re2/src/re2/re2.h"

namespace query_filter {

static constexpr auto kSimpleQueryStringTrackers =
    base::MakeFixedFlatSet<std::string_view>(
        {// https://github.com/brave/brave-browser/issues/4239
         "fbclid", "gclid", "msclkid", "mc_eid",
         // https://github.com/brave/brave-browser/issues/9879
         "dclid",
         // https://github.com/brave/brave-browser/issues/13644
         "oly_anon_id", "oly_enc_id",
         // https://github.com/brave/brave-browser/issues/11579
         "_openstat",
         // https://github.com/brave/brave-browser/issues/11817
         "vero_conv", "vero_id",
         // https://github.com/brave/brave-browser/issues/13647
         "wickedid",
         // https://github.com/brave/brave-browser/issues/11578
         "yclid",
         // https://github.com/brave/brave-browser/issues/8975
         "__s",
         // https://github.com/brave/brave-browser/issues/17451
         "rb_clickid",
         // https://github.com/brave/brave-browser/issues/17452
         "s_cid",
         // https://github.com/brave/brave-browser/issues/17507
         "ml_subscriber", "ml_subscriber_hash",
         // https://github.com/brave/brave-browser/issues/18020
         "twclid",
         // https://github.com/brave/brave-browser/issues/18758
         "gbraid", "wbraid",
         // https://github.com/brave/brave-browser/issues/9019
         "_hsenc", "__hssc", "__hstc", "__hsfp", "hsCtaTracking",
         // https://github.com/brave/brave-browser/issues/22082
         "oft_id", "oft_k", "oft_lk", "oft_d", "oft_c", "oft_ck", "oft_ids",
         "oft_sk",
         // https://github.com/brave/brave-browser/issues/24988
         "ss_email_id",
         // https://github.com/brave/brave-browser/issues/25238
         "bsft_uid", "bsft_clkid",
         // https://github.com/brave/brave-browser/issues/25691
         "guce_referrer", "guce_referrer_sig",
         // https://github.com/brave/brave-browser/issues/31084
         "mtm_cid", "pk_cid",
         // https://github.com/brave/brave-browser/issues/26295
         "vgo_ee"});

static constexpr auto kConditionalQueryStringTrackers =
    base::MakeFixedFlatMap<std::string_view, std::string_view>({
        // https://github.com/brave/brave-browser/issues/9018
        {"mkt_tok", "([uU]nsubscribe|emailWebview)"},
        // https://github.com/brave/brave-browser/issues/30731
        {"h_sid", "/email/"},
        {"h_slt", "/email/"},
    });

static constexpr auto kScopedQueryStringTrackers =
    base::MakeFixedFlatMap<std::string_view, std::string_view>({
        // https://github.com/brave/brave-browser/issues/11580
        {"igshid", "instagram.com"},
        // https://github.com/brave/brave-browser/issues/26966
        {"ref_src", "twitter.com"},
        {"ref_url", "twitter.com"},
    });

// Remove tracking query parameters from a GURL, leaving all
// other parts untouched.
absl::optional<std::string> StripQueryParameter(const std::string_view& query,
                                                const std::string& spec) {
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
  for (const auto& kv_string : input_kv_strings) {
    const std::vector<std::string_view> pieces = SplitStringPiece(
        kv_string, "=", base::KEEP_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
    const std::string_view& key = pieces.empty() ? "" : pieces[0];
    if (pieces.size() >= 2 &&
        (kSimpleQueryStringTrackers.count(key) == 1 ||
         (kScopedQueryStringTrackers.count(key) == 1 &&
          GURL(spec).DomainIs(kScopedQueryStringTrackers.at(key).data())) ||
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
  return absl::nullopt;
}

absl::optional<GURL> ApplyQueryFilter(const GURL& original_url) {
  const auto& query = original_url.query_piece();
  const std::string& spec = original_url.spec();
  const auto clean_query_value = StripQueryParameter(query, spec);
  if (!clean_query_value.has_value()) {
    return absl::nullopt;
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
  return absl::nullopt;
}

absl::optional<GURL> MaybeApplyQueryStringFilter(
    const GURL& initiator_url,
    const GURL& redirect_source_url,
    const GURL& request_url,
    const std::string& request_method,
    const bool internal_redirect) {
  if (!request_url.has_query()) {
    // Optimization:
    // If there are no query params then we have nothing to strip.
    return absl::nullopt;
  }
  if (request_method != "GET") {
    return absl::nullopt;
  }

  if (redirect_source_url.is_valid()) {
    if (internal_redirect) {
      // Ignore internal redirects since we trigger them.
      return absl::nullopt;
    }

    if (net::registry_controlled_domains::SameDomainOrHost(
            redirect_source_url, request_url,
            net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES)) {
      // Same-site redirects are exempted.
      return absl::nullopt;
    }
  } else if (initiator_url.is_valid() &&
             net::registry_controlled_domains::SameDomainOrHost(
                 initiator_url, request_url,
                 net::registry_controlled_domains::
                     INCLUDE_PRIVATE_REGISTRIES)) {
    // Same-site requests are exempted.
    return absl::nullopt;
  }

  return ApplyQueryFilter(request_url);
}
}  // namespace query_filter
