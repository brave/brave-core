/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_service_key_network_delegate_helper.h"

#include <string>
#include <string_view>
#include <vector>

#include "base/containers/fixed_flat_set.h"
#include "base/no_destructor.h"
#include "brave/components/brave_rewards/core/buildflags/buildflags.h"
#include "brave/components/brave_search/common/buildflags/buildflags.h"
#include "brave/components/brave_service_keys/brave_service_key_utils.h"
#include "brave/components/constants/brave_services_key.h"
#include "brave/components/constants/network_constants.h"
#include "brave/components/update_client/buildflags.h"
#include "net/base/net_errors.h"
#include "net/http/http_request_headers.h"
#include "url/gurl.h"

#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
#include "brave/brave_domains/urls.h"
#endif

namespace brave {

namespace {

constexpr char kBraveServiceSignatureHeader[] = "BraveServiceSignature";
constexpr auto kSearchDomains = base::MakeFixedFlatSet<std::string_view>(
    {"search.brave.com", "search.brave.software"});
constexpr auto kTranslateDomains = base::MakeFixedFlatSet<std::string_view>(
    {"translate.brave.com", "translate.bravesoftware.com"});

}  // namespace

template <template <typename> class T>
int OnBeforeStartTransaction_BraveServiceKey(
    net::HttpRequestHeaders* headers,
    const ResponseCallback& next_callback,
    T<BraveRequestInfo> ctx) {
  static const base::NoDestructor<std::vector<std::string>> allowed_domains{{
      kExtensionUpdaterDomain,
      std::string(GURL(BUILDFLAG(UPDATER_DEV_ENDPOINT)).host()),
      std::string(GURL(BUILDFLAG(UPDATER_PROD_ENDPOINT)).host()),
// Gate3 is used by both Rewards and Wallet, but only Rewards OAuth requests
// go through this network delegate path. Wallet gate3 requests use
// APIRequestHelper which adds the services key explicitly.
#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
      std::string(brave_domains::GetGate3URL().host()),
#endif
  }};

  const GURL& url = ctx->request_url();

  if (!url.SchemeIs(url::kHttpsScheme)) {
    return net::OK;
  }

  const bool is_search_domain = kSearchDomains.contains(url.host());
  const bool is_translate_domain =
      kTranslateDomains.contains(url.host()) &&
      ctx->method() == net::HttpRequestHeaders::kPostMethod;
  if (is_search_domain || is_translate_domain ||
      std::any_of(allowed_domains->begin(), allowed_domains->end(),
                  [&url](const auto& domain) { return url.DomainIs(domain); })) {
    headers->SetHeader(kBraveServicesKeyHeader, BUILDFLAG(BRAVE_SERVICES_KEY));
  }
  if (is_search_domain) {
    const auto [_, signature] = brave_service_keys::GetAuthorizationHeader(
        BUILDFLAG(SERVICE_KEY_SEARCH), /*headers=*/{}, url, ctx->method(),
        {"(request-target)"});
    headers->SetHeader(kBraveServiceSignatureHeader, signature);
  }
  if (is_translate_domain) {
    const auto [name, value] = brave_service_keys::GetAuthorizationHeader(
        BUILDFLAG(SERVICE_KEY_SEARCH), /*headers=*/{}, url, ctx->method(),
        {"(request-target)"});
    headers->SetHeader(name, value);
  }
  return net::OK;
}

template int OnBeforeStartTransaction_BraveServiceKey<std::shared_ptr>(
    net::HttpRequestHeaders* headers,
    const ResponseCallback& next_callback,
    std::shared_ptr<BraveRequestInfo> ctx);

template int OnBeforeStartTransaction_BraveServiceKey<base::WeakPtr>(
    net::HttpRequestHeaders* headers,
    const ResponseCallback& next_callback,
    base::WeakPtr<BraveRequestInfo> ctx);

}  // namespace brave
