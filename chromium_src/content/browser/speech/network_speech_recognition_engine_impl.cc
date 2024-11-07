/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "content/browser/speech/network_speech_recognition_engine_impl.h"

#include <string>

#include "brave/components/constants/brave_services_key.h"
#include "brave/components/speech_to_text/features.h"
#include "services/network/public/cpp/resource_request.h"
#include "url/gurl.h"

namespace content::google_apis {
std::string GetAPIKey() {
  return BUILDFLAG(BRAVE_SERVICES_KEY);
}
}  // namespace content::google_apis

#define NetworkSpeechRecognitionEngineImpl \
  NetworkSpeechRecognitionEngineImpl_ChromiumImpl

#include "src/content/browser/speech/network_speech_recognition_engine_impl.cc"

#undef NetworkSpeechRecognitionEngineImpl

namespace content {

NetworkSpeechRecognitionEngineImpl::NetworkSpeechRecognitionEngineImpl(
    scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory)
    : NetworkSpeechRecognitionEngineImpl_ChromiumImpl(
          shared_url_loader_factory),
      shared_url_loader_factory_(std::move(shared_url_loader_factory)) {}

NetworkSpeechRecognitionEngineImpl::~NetworkSpeechRecognitionEngineImpl() =
    default;

void NetworkSpeechRecognitionEngineImpl::StartRecognition() {
  if (!base::FeatureList::IsEnabled(stt::kSttFeature)) {
    return NetworkSpeechRecognitionEngineImpl_ChromiumImpl::StartRecognition();
  }

  auto sticky_request = std::make_unique<network::ResourceRequest>();
  sticky_request->url = GURL(stt::kSttUrl.Get() + "/sticky");
  sticky_request->credentials_mode = network::mojom::CredentialsMode::kInclude;
  sticky_request->site_for_cookies =
      net::SiteForCookies::FromUrl(sticky_request->url);
  sticky_request->method = "GET";

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
