/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_search/browser/brave_search_fallback_host.h"

#include <utility>

#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_search/browser/backup_results_service.h"
#include "net/base/url_util.h"

namespace {
static GURL backup_provider_for_test;
}  // namespace

namespace brave_search {

void BraveSearchFallbackHost::SetBackupProviderForTest(
    const GURL& backup_provider) {
  backup_provider_for_test = backup_provider;
}

BraveSearchFallbackHost::BraveSearchFallbackHost(
    brave_search::BackupResultsService* backup_results_service)
    : backup_results_service_(backup_results_service->GetWeakPtr()),
      weak_factory_(this) {}

BraveSearchFallbackHost::~BraveSearchFallbackHost() = default;

// [static]
GURL BraveSearchFallbackHost::GetBackupResultURL(const GURL& baseURL,
                                                 const std::string& query,
                                                 const std::string& lang,
                                                 const std::string& country,
                                                 const std::string& geo,
                                                 bool filter_explicit_results,
                                                 int page_index) {
  GURL url = baseURL;
  url = net::AppendQueryParameter(url, "q", query);
  url =
      net::AppendQueryParameter(url, "start", base::NumberToString(page_index));
  if (!lang.empty()) {
    url = net::AppendQueryParameter(url, "hl", lang);
  }
  if (!country.empty()) {
    url = net::AppendQueryParameter(url, "gl", country);
  }
  if (filter_explicit_results) {
    url = net::AppendQueryParameter(url, "safe", "active");
  }
  return url;
}

void BraveSearchFallbackHost::FetchBackupResults(
    const std::string& query,
    const std::string& lang,
    const std::string& country,
    const std::string& geo,
    bool filter_explicit_results,
    int page_index,
    const std::optional<std::string>& cookie_header_value,
    FetchBackupResultsCallback callback) {
  auto url = GURL("https://www.google.com/search");
  if (!backup_provider_for_test.is_empty()) {
    url = backup_provider_for_test;
  }
  url = GetBackupResultURL(url, query, lang, country, geo,
                           filter_explicit_results, page_index);

  net::HttpRequestHeaders headers;

  headers.SetHeaderIfMissing("x-geo", geo);
  if (cookie_header_value) {
    headers.SetHeader(net::HttpRequestHeaders::kCookie, *cookie_header_value);
  }

  backup_results_service_->FetchBackupResults(
      url, headers,
      base::BindOnce(&BraveSearchFallbackHost::OnResultsAvailable,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void BraveSearchFallbackHost::OnResultsAvailable(
    BraveSearchFallbackHost::FetchBackupResultsCallback callback,
    const std::optional<BackupResultsService::BackupResults> backup_results) {
  if (backup_results) {
    std::move(callback).Run(backup_results->html);
    return;
  }
  std::move(callback).Run("");
}

}  // namespace brave_search
