/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/base/unittest/unittest_url_response_util.h"

#include <string>

#include "base/check_op.h"
#include "base/containers/flat_map.h"
#include "base/files/file_util.h"
#include "base/no_destructor.h"
#include "base/notreached.h"
#include "base/strings/string_util.h"
#include "bat/ads/internal/base/unittest/unittest_file_util.h"
#include "bat/ads/internal/base/unittest/unittest_tag_parser_util.h"
#include "bat/ads/internal/base/unittest/unittest_test_suite_util.h"
#include "bat/ads/internal/base/unittest/unittest_url_response_headers_util.h"
#include "url/gurl.h"

namespace ads {

namespace {

// Mocked URL responses for the specified URL requests can be defined inline or
// read from a file. Filenames should begin with a forward slash, i.e.
//
//    {
//      "/foo/bar", {
//        {
//          net::HTTP_NOT_FOUND, "Not found"
//        },
//        {
//          net::HTTP_OK, "/response.json"
//        }
//      }
//    }
//
// Inline responses can contain |<time:period>| tags for mocking timestamps,
// where |period| can be |now|, |distant_past|, |distant_future|, |+/-#
// seconds|, |+/-# minutes|, |+/-# hours| or |+/-# days|, i.e.
//
//    {
//      "/foo/bar", {
//        {
//          net::HTTP_OK, "An example response with a <time:+7 days> timestamp"
//        }
//      }
//    }
//
// Multiple URL responses can be added for URL requests which will be returned
// in the specified order, i.e.
//
//    {
//      "/foo/bar", {
//        {
//           net::HTTP_INTERNAL_SERVER_ERROR, "Internal server error"
//        },
//        {
//           net::HTTP_CREATED, "To me there's no creativity without boundaries"
//        }
//      }
//    }

base::flat_map<std::string, size_t>& UrlResponseIndexes() {
  static base::NoDestructor<base::flat_map<std::string, size_t>> indexes;
  return *indexes;
}

URLResponseList GetUrlResponsesForRequestPath(
    const URLResponseMap& url_responses,
    const std::string& request_path) {
  const auto iter = url_responses.find(request_path);
  if (iter == url_responses.end()) {
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
  if (iter == UrlResponseIndexes().end()) {
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
  return base::StartsWith(response_body, "/",
                          base::CompareCase::INSENSITIVE_ASCII);
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
    const std::string filename = ParseFilenameFromResponseBody(response_body);
    const base::FilePath file_path = GetTestPath().AppendASCII(filename);
    if (!base::ReadFileToString(file_path, &response_body)) {
      NOTREACHED() << file_path << " not found";
      return absl::nullopt;
    }

    ParseAndReplaceTags(&response_body);
  }

  return mojom::UrlResponseInfo(url_request->url, url_response->first,
                                response_body,
                                ToUrlResponseHeaders(url_request->headers));
}

}  // namespace ads
