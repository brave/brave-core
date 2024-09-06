// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/query_filter/utils.h"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "base/containers/contains.h"
#include "base/containers/fixed_flat_map.h"
#include "base/containers/fixed_flat_set.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "third_party/re2/src/re2/re2.h"

namespace {

static constexpr auto kSimpleQueryStringTrackers =
    base::MakeFixedFlatSet<std::string_view>(
        base::sorted_unique,
        {
            // https://github.com/brave/brave-browser/issues/9019
            "__hsfp",
            "__hssc",
            "__hstc",
            // https://github.com/brave/brave-browser/issues/8975
            "__s",
            // https://github.com/brave/brave-browser/issues/40716
            "_bhlid",
            // https://github.com/brave/brave-browser/issues/39575
            "_branch_match_id",
            "_branch_referrer",
            // https://github.com/brave/brave-browser/issues/33188
            "_gl",
            // https://github.com/brave/brave-browser/issues/9019
            "_hsenc",
            // https://github.com/brave/brave-browser/issues/34578
            "_kx",
            // https://github.com/brave/brave-browser/issues/11579
            "_openstat",
            // https://github.com/brave/brave-browser/issues/32488
            "at_recipient_id",
            "at_recipient_list",
            // https://github.com/brave/brave-browser/issues/37971
            "bbeml",
            // https://github.com/brave/brave-browser/issues/25238
            "bsft_clkid",
            "bsft_uid",
            // https://github.com/brave/brave-browser/issues/9879
            "dclid",
            // https://github.com/brave/brave-browser/issues/37847
            "et_rid",
            // https://github.com/brave/brave-browser/issues/33984
            "fb_action_ids",
            "fb_comment_id",
            // https://github.com/brave/brave-browser/issues/4239
            "fbclid",
            // https://github.com/brave/brave-browser/issues/18758
            "gbraid",
            // https://github.com/brave/brave-browser/issues/4239
            "gclid",
            // https://github.com/brave/brave-browser/issues/25691
            "guce_referrer",
            "guce_referrer_sig",
            // https://github.com/brave/brave-browser/issues/9019
            "hsCtaTracking",
            // https://github.com/brave/brave-browser/issues/33952
            "irclickid",
            // https://github.com/brave/brave-browser/issues/4239
            "mc_eid",
            // https://github.com/brave/brave-browser/issues/17507
            "ml_subscriber",
            "ml_subscriber_hash",
            // https://github.com/brave/brave-browser/issues/4239
            "msclkid",
            // https://github.com/brave/brave-browser/issues/31084
            "mtm_cid",
            // https://github.com/brave/brave-browser/issues/22082
            "oft_c",
            "oft_ck",
            "oft_d",
            "oft_id",
            "oft_ids",
            "oft_k",
            "oft_lk",
            "oft_sk",
            // https://github.com/brave/brave-browser/issues/13644
            "oly_anon_id",
            "oly_enc_id",
            // https://github.com/brave/brave-browser/issues/31084
            "pk_cid",
            // https://github.com/brave/brave-browser/issues/17451
            "rb_clickid",
            // https://github.com/brave/brave-browser/issues/17452
            "s_cid",
            // https://github.com/brave/brave-browser/issues/40912
            "srsltid",
            // https://github.com/brave/brave-browser/issues/24988
            "ss_email_id",
            // https://github.com/brave/brave-browser/issues/18020
            "twclid",
            // https://github.com/brave/brave-browser/issues/33172
            "unicorn_click_id",
            // https://github.com/brave/brave-browser/issues/11817
            "vero_conv",
            "vero_id",
            // https://github.com/brave/brave-browser/issues/26295
            "vgo_ee",
            // https://github.com/brave/brave-browser/issues/18758
            "wbraid",
            // https://github.com/brave/brave-browser/issues/13647
            "wickedid",
            // https://github.com/brave/brave-browser/issues/11578
            "yclid",
            // https://github.com/brave/brave-browser/issues/33216
            "ymclid",
            "ysclid",
        });

static constexpr auto kConditionalQueryStringTrackers =
    base::MakeFixedFlatMap<std::string_view, std::string_view>({
        // https://github.com/brave/brave-browser/issues/30731
        {"h_sid", "/email/"},
        {"h_slt", "/email/"},
        // https://github.com/brave/brave-browser/issues/9018
        {"mkt_tok", "([uU]nsubscribe|emailWebview)"},
    });

// The second parameter is a comma-separated list of domains.
// The domain comparison will also match on subdomains. So if the
// parameter is scoped to example.com below, it will be removed from
// https://example.com/index.php and from http://www.example.com/ for
// example.
static const auto kScopedQueryStringTrackers =
    std::map<std::string_view, std::vector<std::string_view>>({
        // https://github.com/brave/brave-browser/issues/35094
        {"igsh", {"instagram.com"}},
        // https://github.com/brave/brave-browser/issues/11580
        {"igshid", {"instagram.com"}},
        // https://github.com/brave/brave-browser/issues/26966
        {"ref_src", {"twitter.com", "x.com"}},
        {"ref_url", {"twitter.com", "x.com"}},
        // https://github.com/brave/brave-browser/issues/34719
        {"si", {"youtube.com", "youtu.be"}},
    });

bool IsScopedTracker(
    const std::string_view param_name,
    const std::string& spec,
    const std::map<std::string_view, std::vector<std::string_view>>& trackers) {
  if (!base::Contains(trackers, param_name)) {
    return false;
  }

  const auto& domain_strings = trackers.at(param_name);
  if (domain_strings.empty()) {
    return false;
  }
  const GURL original_url = GURL(spec);
  for (const auto& domain : domain_strings) {
    if (original_url.DomainIs(domain)) {
      return true;
    }
  }

  return false;
}

// Remove tracking query parameters from a GURL, leaving all
// other parts untouched.
std::optional<std::string> StripQueryParameter(const std::string_view query,
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
    const std::string_view key = pieces.empty() ? "" : pieces[0];
    if (pieces.size() >= 2 &&
        (kSimpleQueryStringTrackers.count(key) == 1 ||
         IsScopedTracker(key, spec, kScopedQueryStringTrackers) ||
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
  const auto& query = original_url.query_piece();
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

  return ApplyQueryFilter(request_url);
}

bool IsScopedTrackerForTesting(
    const std::string_view param_name,
    const std::string& spec,
    const std::map<std::string_view, std::vector<std::string_view>>& trackers) {
  return IsScopedTracker(param_name, spec, trackers);
}
}  // namespace query_filter
