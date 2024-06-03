/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "content/browser/speech/network_speech_recognition_engine_impl.h"

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
  request->credentials_mode = network::mojom::CredentialsMode::kInclude;
  request->site_for_cookies = net::SiteForCookies::FromUrl(request->url);

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

#define NetworkSpeechRecognitionEngineImpl \
  NetworkSpeechRecognitionEngineImpl_ChromiumImpl

#include "src/content/browser/speech/network_speech_recognition_engine_impl.cc"

#undef NetworkSpeechRecognitionEngineImpl
#undef downstream_url
#undef upstream_url
#undef BRAVE_NETWORK_SPEECH_RECOGNITION_ENGINE_IMPL_BRAVE_DOWNSTREAM
#undef BRAVE_NETWORK_SPEECH_RECOGNITION_ENGINE_IMPL_BRAVE_UPSTREAM

namespace content {

NetworkSpeechRecognitionEngineImpl::NetworkSpeechRecognitionEngineImpl(
    scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory,
    const std::string& accept_language)
    : NetworkSpeechRecognitionEngineImpl_ChromiumImpl(shared_url_loader_factory,
                                                      accept_language),
      shared_url_loader_factory_(std::move(shared_url_loader_factory)) {}

NetworkSpeechRecognitionEngineImpl::~NetworkSpeechRecognitionEngineImpl() =
    default;

void NetworkSpeechRecognitionEngineImpl::StartRecognition() {
  if (!base::FeatureList::IsEnabled(stt::kSttFeature)) {
    return NetworkSpeechRecognitionEngineImpl_ChromiumImpl::StartRecognition();
  }

  auto sticky_request = std::make_unique<network::ResourceRequest>();
  sticky_request->url =
      GURL(GetWebServiceBaseUrl(web_service_base_url_for_tests) + "/");
  sticky_request->credentials_mode = network::mojom::CredentialsMode::kInclude;
  sticky_request->site_for_cookies =
      net::SiteForCookies::FromUrl(sticky_request->url);
  sticky_request->method = "GET";
  AddBraveHeaders(sticky_request.get(), "sticky_session_request");

  constexpr net::NetworkTrafficAnnotationTag kStickyAnnotationTag =
      net::DefineNetworkTrafficAnnotation("speech_sticky_session_request", R"(
        semantics {
          sender: "Speech Recognition"
          description:
            "In response to this request, load balancer sets special cookies "
            "that will allow further upstream and downstream requests to get "
            "to the same recognition node."
          trigger:
            "The user chooses to start the recognition by clicking the "
            "microphone icon of the pages using Web SpeechRecognition API."
          data: "Brave Service Key V2"
          destination: BRAVE_OWNED_SERVICE
        }
        policy {
          cookies_allowed: YES
          cookies_store: "web contents"
          setting:
            "The user must allow the browser to access the microphone in a "
            "permission prompt. This is set per site (hostname pattern). In "
            "the site settings menu, microphone access can be turned off "
            "for all sites and site specific settings can be changed."
        })");

  auto loader = network::SimpleURLLoader::Create(std::move(sticky_request),
                                                 kStickyAnnotationTag);

  auto* loader_raw = loader.get();
  loader_raw->DownloadToString(
      shared_url_loader_factory_.get(),
      base::BindOnce(&NetworkSpeechRecognitionEngineImpl::OnStickySessionReady,
                     weak_ptr_factory_.GetWeakPtr(), std::move(loader)),
      256);
}

void NetworkSpeechRecognitionEngineImpl::OnStickySessionReady(
    std::unique_ptr<network::SimpleURLLoader> loader,
    std::optional<std::string> response_body) {
  NetworkSpeechRecognitionEngineImpl_ChromiumImpl::StartRecognition();
}

}  // namespace content
