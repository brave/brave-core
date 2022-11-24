/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_query_filter.h"

#include <string>
#include <vector>

#include "base/containers/fixed_flat_map.h"
#include "base/containers/fixed_flat_set.h"
#include "base/strings/string_piece.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "third_party/re2/src/re2/re2.h"
#include "url/gurl.h"

namespace {

static constexpr auto kSimpleQueryStringTrackers =
    base::MakeFixedFlatSet<base::StringPiece>(
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
         // https://github.com/brave/brave-browser/issues/26295
         "vgo_ee"});

static constexpr auto kConditionalQueryStringTrackers =
    base::MakeFixedFlatMap<base::StringPiece, base::StringPiece>(
        {// https://github.com/brave/brave-browser/issues/9018
         {"mkt_tok", "[uU]nsubscribe"}});

static constexpr auto kScopedQueryStringTrackers =
    base::MakeFixedFlatMap<base::StringPiece, base::StringPiece>({
        // https://github.com/brave/brave-browser/issues/11580
        {"igshid", "instagram.com"},
        // https://github.com/brave/brave-browser/issues/26756
        {"t", "twitter.com"},
    });

// Remove tracking query parameters from a GURL, leaving all
// other parts untouched.
absl::optional<std::string> StripQueryParameter(const base::StringPiece& query,
                                                const std::string& spec) {
  // We are using custom query string parsing code here. See
  // https://github.com/brave/brave-core/pull/13726#discussion_r897712350
  // for more information on why this approach was selected.
  //
  // Split query string by ampersands, remove tracking parameters,
  // then join the remaining query parameters, untouched, back into
  // a single query string.
  const std::vector<base::StringPiece> input_kv_strings =
      SplitStringPiece(query, "&", base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);
  std::vector<base::StringPiece> output_kv_strings;
  int disallowed_count = 0;
  for (const auto& kv_string : input_kv_strings) {
    const std::vector<base::StringPiece> pieces = SplitStringPiece(
        kv_string, "=", base::KEEP_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
    const base::StringPiece& key = pieces.empty() ? "" : pieces[0];
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

}  // namespace

absl::optional<GURL> ApplyQueryFilter(const GURL& original_url) {
  const auto& query = original_url.query_piece();
  const std::string& spec = original_url.spec();
  const auto clean_query_value = StripQueryParameter(query, spec);
  if (!clean_query_value.has_value())
    return absl::nullopt;
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
