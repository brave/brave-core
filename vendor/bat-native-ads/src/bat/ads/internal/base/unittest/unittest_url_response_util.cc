/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/base/unittest/unittest_url_response_util.h"

#include <cstdint>
#include <string>

#include "base/check_op.h"
#include "base/containers/flat_map.h"
#include "base/files/file_util.h"
#include "base/no_destructor.h"
#include "base/notreached.h"
#include "base/strings/string_piece.h"
#include "bat/ads/internal/base/unittest/unittest_file_util.h"
#include "bat/ads/internal/base/unittest/unittest_tag_parser_util.h"
#include "bat/ads/internal/base/unittest/unittest_test_suite_util.h"
#include "bat/ads/internal/base/unittest/unittest_url_response_headers_util.h"
#include "url/gurl.h"

namespace ads {

namespace {

// A list of endpoints where the response can be inline, i.e.
//
//    {
//      "/foo/bar", {
//        {
//          net::HTTP_OK, "The quick brown fox jumps over the lazy dog"
//        }
//      }
//    }
//
// or read from a file. Filenames should begin with forward slash. i.e.
//
//    {
//      "/foo/bar", {
//        {
//          net::HTTP_OK, "/response.json"
//        }
//      }
//    }
//
// Inline responses can contain |<time:period>| tags for mocking timestamps,
// where |period| should be |now|, |distant_past|, |distant_future|, |+/-#
// seconds|, |+/-# minutes|, |+/-# hours| or |+/-# days|. i.e.
//
//    {
//      "/foo/bar", {
//        {
//          net::HTTP_OK, "An example response with a <time:+7 days> timestamp"
//        }
//      }
//    }
//
// The same endpoint can be added multiple times where responses are returned in
// the specified order, i.e.
//
//    {
//      "/foo/bar", {
//        {
//           net::HTTP_OK, "/response.json"
//        },
//        {
//           net::HTTP_CREATED, "To me there's no creativity without boundaries"
//        }
//      }
//    }

base::flat_map<std::string, uint16_t>& GetUrlEndpointIndexes() {
  static base::NoDestructor<base::flat_map<std::string, uint16_t>> indexes;
  return *indexes;
}

URLEndpointResponseList GetUrlEndpointResponsesForPath(
    const URLEndpointMap& endpoints,
    const std::string& path) {
  const auto iter = endpoints.find(path);
  if (iter == endpoints.end()) {
    return {};
  }

  return iter->second;
}

absl::optional<URLEndpointResponsePair> GetNextUrlEndpointResponse(
    const GURL& url,
    const URLEndpointMap& endpoints) {
  CHECK(url.is_valid()) << "Invalid URL: " << url;
  CHECK(!endpoints.empty()) << "Missing mock for " << url << " endpoint";

  const std::string path = url.PathForRequest();

  const URLEndpointResponseList url_endpoint_responses =
      GetUrlEndpointResponsesForPath(endpoints, path);
  if (url_endpoint_responses.empty()) {
    // URL endpoint responses not found for given path
    return absl::nullopt;
  }

  uint16_t url_endpoint_response_index = 0;

  const std::string uuid = GetUuidForCurrentTestSuiteAndName(path);
  const auto url_endpoint_response_indexes_iter =
      GetUrlEndpointIndexes().find(uuid);

  if (url_endpoint_response_indexes_iter == GetUrlEndpointIndexes().end()) {
    // uuid does not exist so insert a new index set to 0 for the endpoint
    GetUrlEndpointIndexes()[uuid] = url_endpoint_response_index;
  } else {
    url_endpoint_response_indexes_iter->second++;
    if (url_endpoint_response_indexes_iter->second ==
        url_endpoint_responses.size()) {
      url_endpoint_response_indexes_iter->second = 0;
    }

    url_endpoint_response_index = url_endpoint_response_indexes_iter->second;
  }

  CHECK_GE(url_endpoint_response_index, 0);
  CHECK_LT(url_endpoint_response_index, url_endpoint_responses.size());
  return url_endpoint_responses.at(url_endpoint_response_index);
}

bool ShouldReadBodyFromFile(const std::string& body) {
  return base::StartsWith(body, "/", base::CompareCase::INSENSITIVE_ASCII);
}

base::StringPiece GetFilenameFromBody(const std::string& body) {
  return base::TrimString(body, "//", base::TrimPositions::TRIM_LEADING);
}

base::FilePath GetFilePath(const std::string& body) {
  return GetTestPath().AppendASCII(GetFilenameFromBody(body));
}

}  // namespace

absl::optional<mojom::UrlResponse> GetNextUrlResponse(
    const mojom::UrlRequestPtr& url_request,
    const URLEndpointMap& endpoints) {
  const absl::optional<URLEndpointResponsePair> url_endpoint_response_optional =
      GetNextUrlEndpointResponse(url_request->url, endpoints);
  if (!url_endpoint_response_optional) {
    return absl::nullopt;
  }
  const URLEndpointResponsePair& url_endpoint_response =
      url_endpoint_response_optional.value();

  std::string body = url_endpoint_response.second;
  if (ShouldReadBodyFromFile(body)) {
    const base::FilePath file_path = GetFilePath(body);
    if (!base::ReadFileToString(file_path, &body)) {
      NOTREACHED() << file_path << " not found";
      return absl::nullopt;
    }

    ParseAndReplaceTagsForText(&body);
  }

  mojom::UrlResponse url_response;
  url_response.url = url_request->url;
  url_response.status_code = url_endpoint_response.first;
  url_response.body = body;
  url_response.headers = UrlResponseHeadersToMap(url_request->headers);
  return url_response;
}

}  // namespace ads
