/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/url_sanitizer/url_sanitizer.h"

#include "base/containers/fixed_flat_map.h"
#include "base/containers/fixed_flat_set.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "third_party/re2/src/re2/re2.h"

namespace brave {

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
         // https://github.com/brave/brave-browser/issues/11580
         "igshid"});

static constexpr auto kConditionalQueryStringTrackers =
    base::MakeFixedFlatMap<base::StringPiece, base::StringPiece>(
        {// https://github.com/brave/brave-browser/issues/9018
         {"mkt_tok", "[uU]nsubscribe"}});

}

// Remove tracking query parameters from a GURL, leaving all
// other parts untouched.
// static
std::string URLSanitizer::StripQueryParameter(const std::string& query,
                                const std::string& spec) {
  // We are using custom query string parsing code here. See
  // https://github.com/brave/brave-core/pull/13726#discussion_r897712350
  // for more information on why this approach was selected.
  //
  // Split query string by ampersands, remove tracking parameters,
  // then join the remaining query parameters, untouched, back into
  // a single query string.
  const std::vector<std::string> input_kv_strings =
      SplitString(query, "&", base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);
  std::vector<std::string> output_kv_strings;
  int disallowed_count = 0;
  for (const std::string& kv_string : input_kv_strings) {
    const std::vector<std::string> pieces = SplitString(
        kv_string, "=", base::KEEP_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
    const std::string& key = pieces.empty() ? "" : pieces[0];
    if (pieces.size() >= 2 &&
        (kSimpleQueryStringTrackers.count(key) == 1 ||
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
  } else {
    return query;
  }
}

}  // namespace brave
