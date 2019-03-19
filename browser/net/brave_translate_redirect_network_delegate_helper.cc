/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_translate_redirect_network_delegate_helper.h"

#include "brave/common/translate_network_constants.h"
#include "extensions/common/url_pattern.h"

namespace {
const char kTranslateElementLibQuery[] = "client=te_lib";
}

namespace brave {

bool IsTranslateScriptRequest(const GURL& gurl) {
  static std::vector<URLPattern> translate_patterns({
      URLPattern(URLPattern::SCHEME_HTTPS, kTranslateElementMainJSPattern),
      URLPattern(URLPattern::SCHEME_HTTPS, kTranslateMainJSPattern),
      });
  return std::any_of(translate_patterns.begin(), translate_patterns.end(),
      [&gurl](URLPattern pattern) {
      return pattern.MatchesURL(gurl);
      });
}

bool IsTranslateResourceRequest(const GURL& gurl) {
  static std::vector<URLPattern> translate_patterns({
      URLPattern(URLPattern::SCHEME_HTTPS, kTranslateElementMainCSSPattern),
      URLPattern(URLPattern::SCHEME_HTTPS, kTranslateBrandingPNGPattern),
      });
  return std::any_of(translate_patterns.begin(), translate_patterns.end(),
      [&gurl](URLPattern pattern) {
      return pattern.MatchesURL(gurl);
      });
}

bool IsTranslateRequest(const GURL& gurl) {
  static URLPattern translate_pattern =
    URLPattern(URLPattern::SCHEME_HTTPS, kTranslateRequestPattern);
  return translate_pattern.MatchesURL(gurl);
}

bool IsTranslateGen204Request(const GURL& gurl) {
  static URLPattern pattern = URLPattern(URLPattern::SCHEME_HTTPS,
      kTranslateGen204Pattern);
  bool is_te_lib =
    gurl.spec().find(kTranslateElementLibQuery) != std::string::npos;

  return is_te_lib && pattern.MatchesURL(gurl);
}

int OnBeforeURLRequest_TranslateRedirectWork(
    const ResponseCallback& next_callback,
    std::shared_ptr<BraveRequestInfo> ctx) {
  GURL::Replacements replacements;

  // Abort those gen204 requests triggered by translate element library.
  if (IsTranslateGen204Request(ctx->request_url)) {
    return net::ERR_ABORTED;
  }

  // For those translate resouces which might be triggered by translate
  // element library, go through brave's proxy so we won't introduce direct
  // connection to google when using translate element library.
  if (IsTranslateResourceRequest(ctx->request_url)) {
    replacements.SetPathStr(ctx->request_url.path_piece());
    ctx->new_url_spec = GURL(kBraveTranslateEndpoint).ReplaceComponents(replacements).spec();
    return net::OK;
  }

  // For translate scripts and translate requests, only process them if the
  // initiator is https://translate.googleapis.com so we won't process requests
  // which are not from the translate element library.
  if (ctx->initiator_url.spec() != kTranslateInitiatorURL) {
    return net::OK;
  }

  if (IsTranslateScriptRequest(ctx->request_url)) {
    replacements.SetQueryStr(ctx->request_url.query_piece());
    replacements.SetPathStr(ctx->request_url.path_piece());
    ctx->new_url_spec = GURL(kBraveTranslateEndpoint).ReplaceComponents(replacements).spec();
    return net::OK;
  }

  if (IsTranslateRequest(ctx->request_url)) {
    replacements.SetQueryStr(ctx->request_url.query_piece());
    ctx->new_url_spec =
      GURL(kBraveTranslateEndpoint).ReplaceComponents(replacements).spec();
    return net::OK;
  }

  return net::OK;
}

}  // namespace brave
