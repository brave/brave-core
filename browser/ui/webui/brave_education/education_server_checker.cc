/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_education/education_server_checker.h"

#include <utility>

#include "base/strings/string_split.h"
#include "components/language/core/browser/pref_names.h"
#include "components/prefs/pref_service.h"
#include "services/network/public/cpp/header_util.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "url/gurl.h"

namespace brave_education {

namespace {

constexpr int64_t kMaxDownloadBytes = 1024 * 1024;
constexpr base::TimeDelta kTimeoutDuration = base::Seconds(2);

constexpr auto kTrafficAnnotation =
    net::DefineNetworkTrafficAnnotation("brave_education_server_checker",
                                        R"(
      semantics {
        sender: "Brave Education"
        description: "Attempts to fetch the content for a Brave Education
          server page to ensure that it loads successfully."
        trigger:
          "Completing the Brave Welcome UX flow."
        data:
          "No data sent, other than URL of the education server page. "
          "Data does not contain PII."
        destination: BRAVE_OWNED_SERVICE
      }
      policy {
        cookies_allowed: NO
        setting:
          "None"
      }
    )");

bool URLLoadedWithSuccess(const network::SimpleURLLoader& url_loader) {
  if (url_loader.NetError() != net::OK) {
    return false;
  }
  if (!url_loader.ResponseInfo()) {
    return false;
  }
  auto& headers = url_loader.ResponseInfo()->headers;
  if (!headers) {
    return false;
  }
  return network::IsSuccessfulStatus(headers->response_code());
}

}  // namespace

EducationServerChecker::EducationServerChecker(
    PrefService& pref_service,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : pref_service_(pref_service), url_loader_factory_(url_loader_factory) {}

EducationServerChecker::~EducationServerChecker() = default;

void EducationServerChecker::IsServerPageAvailable(
    EducationPageType page_type,
    IsServerPageAvailableCallback callback) {
  auto request = std::make_unique<network::ResourceRequest>();
  request->url = GetEducationPageServerURL(page_type);
  request->referrer_policy = net::ReferrerPolicy::NO_REFERRER;
  request->credentials_mode = network::mojom::CredentialsMode::kOmit;

  auto languages = base::SplitString(
      pref_service_->GetString(language::prefs::kAcceptLanguages), ",",
      base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
  if (!languages.empty()) {
    request->headers.SetHeader(request->headers.kAcceptLanguage,
                               languages.front());
  }

  auto url_loader =
      network::SimpleURLLoader::Create(std::move(request), kTrafficAnnotation);

  url_loader->SetTimeoutDuration(kTimeoutDuration);

  auto* url_loader_ptr = url_loader.get();

  auto download_callback = base::BindOnce(
      &EducationServerChecker::OnURLResponse, weak_factory_.GetWeakPtr(),
      page_type, std::move(url_loader), std::move(callback));

  url_loader_ptr->DownloadToString(url_loader_factory_.get(),
                                   std::move(download_callback),
                                   kMaxDownloadBytes);
}

void EducationServerChecker::OnURLResponse(
    EducationPageType page_type,
    std::unique_ptr<network::SimpleURLLoader> url_loader,
    IsServerPageAvailableCallback callback,
    std::optional<std::string> body) {
  CHECK(url_loader);
  std::move(callback).Run(URLLoadedWithSuccess(*url_loader) && body);
}

}  // namespace brave_education
