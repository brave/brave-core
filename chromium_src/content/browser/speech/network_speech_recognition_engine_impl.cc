/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>

#include "base/containers/flat_map.h"
#include "base/i18n/time_formatting.h"
#include "brave/components/brave_service_keys/brave_service_key_utils.h"
#include "brave/components/constants/brave_services_key.h"
#include "brave/components/speech_to_text/buildflags.h"
#include "brave/components/speech_to_text/features.h"
#include "services/network/public/cpp/resource_request.h"
#include "url/gurl.h"

namespace content::google_apis {
std::string GetAPIKey() {
  return BUILDFLAG(BRAVE_SERVICES_KEY);
}
}  // namespace content::google_apis

namespace {

std::string GetWebServiceBaseUrl(const char* web_service_base_url_for_tests) {
  if (base::FeatureList::IsEnabled(stt::kSttFeature)) {
    return stt::kSttUrl.Get();
  }
  // Fallback to for-tests URL.
  return web_service_base_url_for_tests;
}

void AddBraveHeaders(network::ResourceRequest* request,
                     const std::string& request_key) {
  DCHECK(request && !request->method.empty() && request->url.is_valid());

  const std::string brave_sticky_session_cookie =
      "Brave-stt-sticky=" + request_key;
  request->headers.SetHeader(net::HttpRequestHeaders::kCookie,
                             brave_sticky_session_cookie);

  constexpr const char kRequestKey[] = "request-key";
  request->headers.SetHeader(kRequestKey, request_key);
  constexpr const char kRequestDate[] = "request-date";
  const auto request_date = base::TimeFormatHTTP(base::Time::Now());
  request->headers.SetHeader(kRequestDate, request_date);

  const base::flat_map<std::string, std::string> headers{
      {kRequestKey, request_key}, {kRequestDate, request_date}};
  const auto authorization = brave_service_keys::GetAuthorizationHeader(
      BUILDFLAG(SERVICE_KEY_STT), headers, request->url, request->method,
      {kRequestKey, kRequestDate});
  if (authorization) {
    request->headers.SetHeader(authorization->first, authorization->second);
  }
}

}  // namespace

#define downstream_url(arg)                                             \
  downstream_url(GetWebServiceBaseUrl(web_service_base_url_for_tests) + \
                 std::string(kDownstreamUrl) +                          \
                 base::JoinString(downstream_args, "&"));               \
  web_service_base_url;  // Google service is unused.

#define upstream_url(arg)                                             \
  upstream_url(GetWebServiceBaseUrl(web_service_base_url_for_tests) + \
               std::string(kUpstreamUrl) +                            \
               base::JoinString(upstream_args, "&"))

#define BRAVE_NETWORK_SPEECH_RECOGNITION_ENGINE_IMPL_BRAVE_DOWNSTREAM \
  AddBraveHeaders(downstream_request.get(), request_key);

#define BRAVE_NETWORK_SPEECH_RECOGNITION_ENGINE_IMPL_BRAVE_UPSTREAM \
  AddBraveHeaders(upstream_request.get(), request_key);

#include "src/content/browser/speech/network_speech_recognition_engine_impl.cc"

#undef downstream_url
#undef upstream_url
#undef BRAVE_NETWORK_SPEECH_RECOGNITION_ENGINE_IMPL_BRAVE_DOWNSTREAM
#undef BRAVE_NETWORK_SPEECH_RECOGNITION_ENGINE_IMPL_BRAVE_UPSTREAM
