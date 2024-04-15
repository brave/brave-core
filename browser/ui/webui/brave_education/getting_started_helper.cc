/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_education/getting_started_helper.h"

#include <utility>

#include "brave/components/brave_education/common/features.h"
#include "chrome/browser/net/system_network_context_manager.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/reduce_accept_language_controller_delegate.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/url_response_head.mojom.h"

namespace brave_education {

namespace {

constexpr int64_t kMaxDownloadBytes = 1024 * 1024;

constexpr auto kTrafficAnnotation = net::DefineNetworkTrafficAnnotation(
    "brave_education_getting_started_helper",
    R"(
      semantics {
        sender: "Brave Education"
        description: "Attempts to fetch the content for the Brave Education
          getting-started page to ensure that it loads successfully."
        trigger:
          "Completing the Brave Welcome UX flow."
        data:
          "No data sent, other than URL of the getting-started page. "
          "Data does not contain PII."
        destination: BRAVE_OWNED_SERVICE
      }
      policy {
        cookies_allowed: NO
        setting:
          "None"
      }
    )");

std::optional<EducationContentType> GetEducationContentType() {
  if (base::FeatureList::IsEnabled(features::kShowGettingStartedPage)) {
    return EducationContentType::kGettingStarted;
  }
  return std::nullopt;
}

}  // namespace

GettingStartedHelper::GettingStartedHelper(Profile* profile)
    : profile_(profile) {}

GettingStartedHelper::~GettingStartedHelper() = default;

void GettingStartedHelper::GetEducationURL(GetEducationURLCallback callback) {
  // Add the callback to our list of pending callbacks.
  url_callbacks_.push_back(std::move(callback));

  // If we are currently waiting on a URL, then exit. All callbacks in the
  // pending list will be executed when the website check is completed.
  if (url_loader_) {
    return;
  }

  auto content_type = GetEducationContentType();
  if (!content_type) {
    RunCallbacks(std::nullopt);
    return;
  }

  // Attempt to fetch the content URL in the background.
  auto url_loader_factory = profile_->GetURLLoaderFactory();
  auto request = std::make_unique<network::ResourceRequest>();
  request->url = GetEducationContentServerURL(*content_type);
  request->referrer_policy = net::ReferrerPolicy::NO_REFERRER;
  request->credentials_mode = network::mojom::CredentialsMode::kOmit;

  if (auto* delegate = profile_->GetReduceAcceptLanguageControllerDelegate()) {
    auto languages = delegate->GetUserAcceptLanguages();
    if (!languages.empty()) {
      request->headers.SetHeader(request->headers.kAcceptLanguage,
                                 languages.front());
    }
  }

  url_loader_ =
      network::SimpleURLLoader::Create(std::move(request), kTrafficAnnotation);

  url_loader_->DownloadToString(
      url_loader_factory.get(),
      base::BindOnce(&GettingStartedHelper::OnURLResponse,
                     base::Unretained(this), *content_type),
      kMaxDownloadBytes);
}

void GettingStartedHelper::OnURLResponse(EducationContentType content_type,
                                         std::optional<std::string> body) {
  if (URLLoadedWithSuccess() && body) {
    RunCallbacks(GetEducationContentBrowserURL(content_type));
  } else {
    RunCallbacks(std::nullopt);
  }
}

bool GettingStartedHelper::URLLoadedWithSuccess() {
  if (!url_loader_) {
    return false;
  }
  if (url_loader_->NetError() != net::OK) {
    return false;
  }
  if (!url_loader_->ResponseInfo()) {
    return false;
  }
  auto& headers = url_loader_->ResponseInfo()->headers;
  if (!headers) {
    return false;
  }
  int response_code = headers->response_code();
  return response_code >= 200 && response_code <= 302;
}

void GettingStartedHelper::RunCallbacks(std::optional<GURL> webui_url) {
  url_loader_.reset();
  std::list callbacks = std::move(url_callbacks_);
  for (auto& callback : callbacks) {
    std::move(callback).Run(webui_url);
  }
}

}  // namespace brave_education
