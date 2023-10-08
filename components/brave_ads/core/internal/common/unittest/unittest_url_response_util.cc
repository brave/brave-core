/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_url_response_util.h"

#include <string>

#include "base/check_op.h"
#include "base/containers/flat_map.h"
#include "base/files/file_util.h"
#include "base/no_destructor.h"
#include "base/notreached.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_current_test_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_file_path_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_tag_parser_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_url_response_headers_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "url/gurl.h"

namespace brave_ads {

namespace {

base::flat_map<std::string, size_t>& UrlResponseIndexes() {
  static base::NoDestructor<base::flat_map<std::string, size_t>> indexes;
  return *indexes;
}

URLResponseList GetUrlResponsesForRequestPath(
    const URLResponseMap& url_responses,
    const std::string& request_path) {
  const auto iter = url_responses.find(request_path);
  if (iter == url_responses.cend()) {
    return {};
  }

  return iter->second;
}

absl::optional<URLResponsePair> GetNextUrlResponseForUrl(
    const GURL& url,
    const URLResponseMap& url_responses) {
  CHECK(url.is_valid()) << "Invalid URL: " << url;
  CHECK(!url_responses.empty()) << "Missing mock for " << url << " responses";

  const std::string request_path = url.PathForRequest();

  const URLResponseList url_responses_for_request_path =
      GetUrlResponsesForRequestPath(url_responses, request_path);
  if (url_responses_for_request_path.empty()) {
    // URL responses not found for the given request path.
    return absl::nullopt;
  }

  size_t index = 0;

  const std::string uuid = GetUuidForCurrentTestAndValue(request_path);

  const auto iter = UrlResponseIndexes().find(uuid);
  if (iter == UrlResponseIndexes().cend()) {
    // uuid does not exist so insert a new index set to 0 for the url responses.
    UrlResponseIndexes()[uuid] = index;
  } else {
    iter->second++;
    if (iter->second == url_responses_for_request_path.size()) {
      iter->second = 0;
    }

    index = iter->second;
  }

  CHECK_LT(index, url_responses_for_request_path.size());
  return url_responses_for_request_path.at(index);
}

bool ShouldReadResponseBodyFromFile(const std::string& response_body) {
  return base::StartsWith(response_body, "/");
}

std::string ParseFilenameFromResponseBody(const std::string& response_body) {
  return std::string(
      base::TrimString(response_body, "//", base::TrimPositions::TRIM_LEADING));
}

}  // namespace

absl::optional<mojom::UrlResponseInfo> GetNextUrlResponseForRequest(
    const mojom::UrlRequestInfoPtr& url_request,
    const URLResponseMap& url_responses) {
  const absl::optional<URLResponsePair> url_response =
      GetNextUrlResponseForUrl(url_request->url, url_responses);
  if (!url_response) {
    return absl::nullopt;
  }

  std::string response_body = url_response->second;
  if (ShouldReadResponseBodyFromFile(response_body)) {
    const base::FilePath file_path =
        GetTestPath().AppendASCII(ParseFilenameFromResponseBody(response_body));
    if (!base::ReadFileToString(file_path, &response_body)) {
      NOTREACHED_NORETURN() << file_path << " not found";
    }

    ParseAndReplaceTags(response_body);
  }

  return mojom::UrlResponseInfo(url_request->url, url_response->first,
                                response_body,
                                ToUrlResponseHeaders(url_request->headers));
}

}  // namespace brave_ads
