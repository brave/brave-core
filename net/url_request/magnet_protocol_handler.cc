/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/net/url_request/magnet_protocol_handler.h"

#include "brave/common/network_constants.h"
#include "base/strings/string_util.h"
#include "net/base/escape.h"
#include "net/base/net_errors.h"
#include "net/url_request/url_request_error_job.h"
#include "net/url_request/url_request_redirect_job.h"

MagnetProtocolHandler::MagnetProtocolHandler() {
}

MagnetProtocolHandler::~MagnetProtocolHandler() {
}

GURL MagnetProtocolHandler::TranslateURL(const GURL& url) const {
  GURL extension_page_url(
      "chrome-extension://lgjmpdmojkpocjcopdikifhejkkjglho/extension/brave_webtorrent.html?%s");
  std::string translatedSpec(extension_page_url.spec());
  base::ReplaceFirstSubstringAfterOffset(
      &translatedSpec, 0, "%s",
      net::EscapeQueryParamValue(url.spec(), true));
  return GURL(translatedSpec);
}

net::URLRequestJob* MagnetProtocolHandler::MaybeCreateJob(
    net::URLRequest* request, net::NetworkDelegate* network_delegate) const {

  GURL translated_url(TranslateURL(request->url()));
  if (!translated_url.is_valid()) {
    return new net::URLRequestErrorJob(
        request, network_delegate, net::ERR_INVALID_URL);
  }

  return new net::URLRequestRedirectJob(
      request, network_delegate, translated_url,
      net::URLRequestRedirectJob::REDIRECT_307_TEMPORARY_REDIRECT,
      "WebTorrent");
}
