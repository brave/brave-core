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
#include "base/sequenced_task_runner.h"
#include "base/strings/string_util.h"
#include "brave/common/network_constants.h"
#include "brave/common/shield_exceptions.h"
#include "brave/common/url_constants.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "brave/components/brave_shields/browser/brave_shields_web_contents_observer.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/referrer.h"
#include "extensions/common/url_pattern.h"
#include "net/url_request/url_request.h"
#include "third_party/re2/src/re2/re2.h"

using content::BrowserThread;
using content::Referrer;

namespace brave {

namespace {

const std::string& GetQueryStringTrackers() {
  static const base::NoDestructor<std::string> trackers(base::JoinString(
      std::vector<std::string>({"fbclid", "gclid", "msclkid", "mc_eid"}), "|"));
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

bool ApplyPotentialReferrerBlock(std::shared_ptr<BraveRequestInfo> ctx) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  GURL target_origin = ctx->request_url.GetOrigin();
  GURL tab_origin = ctx->tab_origin;
  if (tab_origin.SchemeIs(kChromeExtensionScheme)) {
    return false;
  }

  const std::string original_referrer = ctx->referrer.spec();
  Referrer new_referrer;
  if (brave_shields::ShouldSetReferrer(
          ctx->allow_referrers, ctx->allow_brave_shields,
          GURL(original_referrer), tab_origin, ctx->request_url, target_origin,
          Referrer::NetReferrerPolicyToBlinkReferrerPolicy(
              ctx->referrer_policy), &new_referrer)) {
    ctx->new_referrer = new_referrer.url;
    return true;
  }
  return false;
}

void ApplyPotentialQueryStringFilter(const GURL& request_url,
                                     std::string* new_url_spec) {
  DCHECK(new_url_spec);
  SCOPED_UMA_HISTOGRAM_TIMER("Brave.SiteHacks.QueryFilter");
  std::string new_query = request_url.query();
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
    *new_url_spec = request_url.ReplaceComponents(replacements).spec();
  }
}

}  // namespace

int OnBeforeURLRequest_SiteHacksWork(
    const ResponseCallback& next_callback,
    std::shared_ptr<BraveRequestInfo> ctx) {
  ApplyPotentialReferrerBlock(ctx);

  if (ctx->request_url.has_query()) {
    ApplyPotentialQueryStringFilter(ctx->request_url, &ctx->new_url_spec);
  }
  return net::OK;
}

int OnBeforeStartTransaction_SiteHacksWork(
    net::HttpRequestHeaders* headers,
    const ResponseCallback& next_callback,
    std::shared_ptr<BraveRequestInfo> ctx) {
  if (IsUAWhitelisted(ctx->request_url)) {
    std::string user_agent;
    if (headers->GetHeader(kUserAgentHeader, &user_agent)) {
      base::ReplaceFirstSubstringAfterOffset(&user_agent, 0,
        "Chrome", "Brave Chrome");
      headers->SetHeader(kUserAgentHeader, user_agent);
      ctx->set_headers.insert(kUserAgentHeader);
    }
  }
  return net::OK;
}

}  // namespace brave
