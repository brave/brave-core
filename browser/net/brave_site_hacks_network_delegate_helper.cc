/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_site_hacks_network_delegate_helper.h"

#include <memory>
#include <string>
#include <vector>

#include "base/lazy_instance.h"
#include "base/metrics/histogram_macros.h"
#include "base/no_destructor.h"
#include "base/strings/string_util.h"
#include "brave/common/url_constants.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "content/public/common/referrer.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "net/url_request/url_request.h"
#include "third_party/blink/public/common/loader/network_utils.h"
#include "third_party/blink/public/common/loader/referrer_utils.h"
#include "third_party/re2/src/re2/re2.h"

namespace brave {

namespace {


const std::string& GetQueryStringTrackers() {
  static const base::NoDestructor<std::string> trackers(base::JoinString(
      std::vector<std::string>(
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
           // https://github.com/brave/brave-browser/issues/9019
           "_hsenc", "__hssc", "__hstc", "__hsfp", "hsCtaTracking"}),
      "|"));
  return *trackers;
}

// From src/components/autofill/content/renderer/page_passwords_analyser.cc
// and password_form_conversion_utils.cc:
#define DECLARE_LAZY_MATCHER(NAME, PATTERN)                                   \
  struct LabelPatternLazyInstanceTraits_##NAME                                \
      : public base::internal::DestructorAtExitLazyInstanceTraits<re2::RE2> { \
    static re2::RE2* New(void* instance) {                                    \
      re2::RE2::Options options;                                              \
      options.set_case_sensitive(false);                                      \
      re2::RE2* matcher = new (instance) re2::RE2(PATTERN, options);          \
      DCHECK(matcher->ok());                                                  \
      return matcher;                                                         \
    }                                                                         \
  };                                                                          \
  base::LazyInstance<re2::RE2, LabelPatternLazyInstanceTraits_##NAME> NAME =  \
      LAZY_INSTANCE_INITIALIZER

// e.g. "?fbclid=1234"
DECLARE_LAZY_MATCHER(tracker_only_matcher,
                     "^(" + GetQueryStringTrackers() + ")=[^&]+$");

// e.g. "?fbclid=1234&foo=1"
DECLARE_LAZY_MATCHER(tracker_first_matcher,
                     "^(" + GetQueryStringTrackers() + ")=[^&]+&");

// e.g. "?foo=1&fbclid=1234" or "?foo=1&fbclid=1234&bar=2"
DECLARE_LAZY_MATCHER(tracker_appended_matcher,
                     "&(" + GetQueryStringTrackers() + ")=[^&]+");

#undef DECLARE_LAZY_MATCHER

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

  // Regular expressions are dirty and error-prone, but unfortunately
  // there is no right way to parse a query string, other than one
  // generated by a URL-encoded HTML form submission. See
  // https://github.com/brave/brave-core/pull/3239#issuecomment-524073918
  // for more information on why this approach was selected.

  std::string new_query = ctx->request_url.query();
  // Note: the ordering of these replacements is important.
  const int replacement_count =
      re2::RE2::GlobalReplace(&new_query, tracker_appended_matcher.Get(), "") +
      re2::RE2::GlobalReplace(&new_query, tracker_first_matcher.Get(), "") +
      re2::RE2::GlobalReplace(&new_query, tracker_only_matcher.Get(), "");

  if (replacement_count > 0) {
    url::Replacements<char> replacements;
    if (new_query.empty()) {
      replacements.ClearQuery();
    } else {
      replacements.SetQuery(new_query.c_str(),
                            url::Component(0, new_query.size()));
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
