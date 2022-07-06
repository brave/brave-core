/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_site_hacks_network_delegate_helper.h"

#include <memory>
#include <string>
#include <vector>

#include "base/containers/fixed_flat_map.h"
#include "base/containers/fixed_flat_set.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/string_piece.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "brave/components/constants/url_constants.h"
#include "content/public/common/referrer.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "net/url_request/url_request.h"
#include "third_party/blink/public/common/loader/network_utils.h"
#include "third_party/blink/public/common/loader/referrer_utils.h"
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

// Remove tracking query parameters from a GURL, leaving all
// other parts untouched.
std::string StripQueryParameter(const std::string& query,
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

void ApplyPotentialQueryStringFilter(std::shared_ptr<BraveRequestInfo> ctx) {
  SCOPED_UMA_HISTOGRAM_TIMER("Brave.SiteHacks.QueryFilter");

  if (!ctx->allow_brave_shields) {
    // Don't apply the filter if the destination URL has shields down.
    return;
  }

  if (ctx->redirect_source.is_valid()) {
    if (ctx->internal_redirect) {
      // Ignore internal redirects since we trigger them.
      return;
    }

    if (net::registry_controlled_domains::SameDomainOrHost(
            ctx->redirect_source, ctx->request_url,
            net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES)) {
      // Same-site redirects are exempted.
      return;
    }
  } else if (ctx->initiator_url.is_valid() &&
             net::registry_controlled_domains::SameDomainOrHost(
                 ctx->initiator_url, ctx->request_url,
                 net::registry_controlled_domains::
                     INCLUDE_PRIVATE_REGISTRIES)) {
    // Same-site requests are exempted.
    return;
  }

  const std::string query = ctx->request_url.query();
  const std::string spec = ctx->request_url.spec();
  const std::string clean_query = StripQueryParameter(query, spec);
  if (clean_query.length() < query.length()) {
    GURL::Replacements replacements;
    if (clean_query.empty()) {
      replacements.ClearQuery();
    } else {
      replacements.SetQueryStr(clean_query);
    }
    ctx->new_url_spec = ctx->request_url.ReplaceComponents(replacements).spec();
  }
}

bool ApplyPotentialReferrerBlock(std::shared_ptr<BraveRequestInfo> ctx) {
  if (ctx->tab_origin.SchemeIs(kChromeExtensionScheme)) {
    return false;
  }

  if (ctx->resource_type == blink::mojom::ResourceType::kMainFrame ||
      ctx->resource_type == blink::mojom::ResourceType::kSubFrame) {
    // Frame navigations are handled in content::NavigationRequest.
    return false;
  }

  content::Referrer new_referrer;
  if (brave_shields::MaybeChangeReferrer(
          ctx->allow_referrers, ctx->allow_brave_shields, GURL(ctx->referrer),
          ctx->request_url, &new_referrer)) {
    ctx->new_referrer = new_referrer.url;
    return true;
  }
  return false;
}

}  // namespace

int OnBeforeURLRequest_SiteHacksWork(const ResponseCallback& next_callback,
                                     std::shared_ptr<BraveRequestInfo> ctx) {
  ApplyPotentialReferrerBlock(ctx);
  if (ctx->request_url.has_query()) {
    ApplyPotentialQueryStringFilter(ctx);
  }
  return net::OK;
}

int OnBeforeStartTransaction_SiteHacksWork(
    net::HttpRequestHeaders* headers,
    const ResponseCallback& next_callback,
    std::shared_ptr<BraveRequestInfo> ctx) {
  // Special case for handling top-level redirects. There is no other way to
  // normally change referrer in net::URLRequest during redirects
  // (except using network::mojom::TrustedURLLoaderHeaderClient, which
  // will affect performance).
  // Note that this code only affects "Referer" header sent via network - we
  // handle document.referer in content::NavigationRequest (see also
  // |BraveContentBrowserClient::MaybeHideReferrer|).
  if (!ctx->allow_referrers && ctx->allow_brave_shields &&
      ctx->redirect_source.is_valid() &&
      ctx->resource_type == blink::mojom::ResourceType::kMainFrame &&
      !brave_shields::IsSameOriginNavigation(ctx->redirect_source,
                                             ctx->request_url)) {
    // This is a hack that notifies the network layer.
    ctx->removed_headers.insert("X-Brave-Cap-Referrer");
  }
  return net::OK;
}

}  // namespace brave
