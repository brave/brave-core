/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/test/internal/url_response_test_util_internal.h"

#include <cstddef>
#include <string>
#include <vector>

#include "base/check_op.h"
#include "base/containers/flat_map.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/no_destructor.h"
#include "base/notreached.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/internal/common/test/file_path_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/internal/current_test_util_internal.h"
#include "brave/components/brave_ads/core/internal/common/test/internal/tag_parser_test_util_internal.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "url/gurl.h"

namespace brave_ads::test {

namespace {

base::flat_map<std::string, size_t>& UrlResponseIndexes() {
  static base::NoDestructor<base::flat_map<std::string, size_t>> indexes;
  return *indexes;
}

URLResponseList GetUrlResponsesForRequestPath(
    const URLResponseMap& url_responses,
    const std::string& url_request_path) {
  const auto iter = url_responses.find(url_request_path);
  if (iter == url_responses.cend()) {
    return {};
  }

  return iter->second;
}

std::optional<URLResponsePair> GetNextUrlResponseForUrl(
    const GURL& url,
    const URLResponseMap& url_responses) {
  CHECK(url.is_valid()) << "Invalid URL: " << url;
  CHECK(!url_responses.empty()) << "Missing mock for " << url << " responses";

  const std::string url_request_path = url.PathForRequest();

  const URLResponseList url_responses_for_request_path =
      GetUrlResponsesForRequestPath(url_responses, url_request_path);
  if (url_responses_for_request_path.empty()) {
    // URL responses not found for the given request path.
    return std::nullopt;
  }

  size_t index = 0;

  const std::string uuid = GetUuidForCurrentTestAndValue(url_request_path);

  const auto iter = UrlResponseIndexes().find(uuid);
  if (iter == UrlResponseIndexes().cend()) {
    // uuid does not exist so insert a new index set to 0 for the url responses.
    UrlResponseIndexes()[uuid] = index;
  } else {
    ++iter->second;
    if (iter->second == url_responses_for_request_path.size()) {
      iter->second = 0;
    }

    index = iter->second;
  }

  CHECK_LT(index, url_responses_for_request_path.size());
  return url_responses_for_request_path.at(index);
}

bool ShouldReadResponseBodyFromFile(const std::string& response_body) {
  return response_body.starts_with("/");
}

std::string ParseFilenameFromResponseBody(const std::string& response_body) {
  return std::string(
      base::TrimString(response_body, "//", base::TrimPositions::TRIM_LEADING));
}

base::flat_map<std::string, std::string> ToUrlResponseHeaders(
    const std::vector<std::string>& headers) {
  base::flat_map<std::string, std::string> response_headers;

  for (const auto& header : headers) {
    const std::vector<std::string> components = base::SplitString(
        header, ":", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
    CHECK_EQ(2U, components.size()) << "Invalid header: " << header;

    const std::string& name = components.at(0);
    const std::string& value = components.at(1);

    response_headers[name] = value;
  }

  return response_headers;
}

}  // namespace

std::optional<mojom::UrlResponseInfo> GetNextUrlResponseForRequest(
    const mojom::UrlRequestInfoPtr& mojom_url_request,
    const URLResponseMap& url_responses) {
  const std::optional<URLResponsePair> url_response =
      GetNextUrlResponseForUrl(mojom_url_request->url, url_responses);
  if (!url_response) {
    return std::nullopt;
  }

  auto [response_status_code, response_body] = *url_response;

  if (ShouldReadResponseBodyFromFile(response_body)) {
    const base::FilePath path = UrlResponsesDataPath().AppendASCII(
        ParseFilenameFromResponseBody(response_body));
    if (!base::ReadFileToString(path, &response_body)) {
      NOTREACHED_NORETURN() << path << " not found";
    }

    ParseAndReplaceTags(response_body);
  }

  return mojom::UrlResponseInfo(
      mojom_url_request->url, response_status_code, response_body,
      ToUrlResponseHeaders(mojom_url_request->headers));
}

}  // namespace brave_ads::test
