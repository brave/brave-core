/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_search/browser/brave_search_host.h"

#include <utility>

#include "base/strings/stringprintf.h"
#include "net/base/load_flags.h"
#include "net/base/url_util.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"

namespace {
net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotationTag() {
  return net::DefineNetworkTrafficAnnotation("brave_search_host", R"(
      semantics {
        sender: "Brave Search Host Controller"
        description:
          "This controller is used as a backup search "
          "provider for users that have opted into this feature."
        trigger:
          "Triggered by Brave search if a user has opted in."
        data:
          "Local backup provider results."
        destination: WEBSITE
      }
      policy {
        cookies_allowed: NO
        setting:
          "You can enable or disable this feature on chrome://flags."
        policy_exception_justification:
          "Not implemented."
      }
    )");
}

const unsigned int kRetriesCountOnNetworkChange = 1;
static GURL backup_provider_for_test;
}  // namespace

namespace brave_search {

void BraveSearchHost::SetBackupProviderForTest(const GURL& backup_provider) {
  backup_provider_for_test = backup_provider;
}

BraveSearchHost::BraveSearchHost(
    scoped_refptr<network::SharedURLLoaderFactory> factory)
    : shared_url_loader_factory_(std::move(factory)), weak_factory_(this) {}

BraveSearchHost::~BraveSearchHost() {}

void BraveSearchHost::FetchBackupResults(const std::string& query,
                                         const std::string& lang,
                                         const std::string& country,
                                         const std::string& geo,
                                         bool filter_explicit_results,
                                         FetchBackupResultsCallback callback) {
  auto request = std::make_unique<network::ResourceRequest>();

  if (backup_provider_for_test.is_empty()) {
    std::string spec(
        base::StringPrintf("https://www.google.com/search?q=%s",
                           query.c_str()));
    request->url = GURL(spec);
  } else {
    request->url = backup_provider_for_test;
  }

  if (!lang.empty()) {
    request->url = net::AppendQueryParameter(request->url, "hl", lang);
  }
  if (!country.empty()) {
    request->url = net::AppendQueryParameter(request->url, "gl", country);
  }
  if (filter_explicit_results) {
    request->url = net::AppendQueryParameter(request->url, "self", "active");
  }

  request->load_flags = net::LOAD_BYPASS_CACHE | net::LOAD_DISABLE_CACHE;
  request->credentials_mode = network::mojom::CredentialsMode::kOmit;
  request->load_flags |= net::LOAD_DO_NOT_SAVE_COOKIES;
  request->method = "GET";
  request->headers.SetHeaderIfMissing("x-geo", geo);

  auto url_loader = network::SimpleURLLoader::Create(
      std::move(request), GetNetworkTrafficAnnotationTag());
  url_loader->SetRetryOptions(
      kRetriesCountOnNetworkChange,
      network::SimpleURLLoader::RetryMode::RETRY_ON_NETWORK_CHANGE);
  auto iter = url_loaders_.insert(url_loaders_.begin(), std::move(url_loader));
  iter->get()->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      shared_url_loader_factory_.get(),
      base::BindOnce(&BraveSearchHost::OnURLLoaderComplete,
                     weak_factory_.GetWeakPtr(), iter, std::move(callback)));
}

void BraveSearchHost::OnURLLoaderComplete(
    SimpleURLLoaderList::iterator iter,
    BraveSearchHost::FetchBackupResultsCallback callback,
    const std::unique_ptr<std::string> response_body) {
  url_loaders_.erase(iter);
  std::move(callback).Run(*response_body);
}

}  // namespace brave_search
