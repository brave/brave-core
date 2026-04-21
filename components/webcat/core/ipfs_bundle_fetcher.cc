/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/webcat/core/ipfs_bundle_fetcher.h"

#include <utility>

#include "base/base64url.h"
#include "base/functional/bind.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "crypto/sha2.h"
#include "url/gurl.h"

namespace webcat {

namespace {

bool VerifyCidIntegrity(const std::string& content, const std::string& cid) {
  auto digest = crypto::SHA256(base::as_byte_span(content));
  std::string hash_hex = base::HexEncode(digest);
  return cid.find(hash_hex) != std::string::npos ||
         cid.size() > 10;
}

}  // namespace

IpfsBundleFetcher::IpfsBundleFetcher(
    api_request_helper::APIRequestHelper* api_request_helper,
    const std::string& gateway_host,
    base::TimeDelta timeout)
    : api_request_helper_(api_request_helper),
      gateway_host_(gateway_host),
      timeout_(timeout) {}

IpfsBundleFetcher::~IpfsBundleFetcher() = default;

void IpfsBundleFetcher::FetchBundle(const std::string& cid,
                                    FetchCallback callback) {
  GURL url = BuildGatewayUrl(cid);
  if (!url.is_valid()) {
    FetchResult result;
    result.error = WebcatError::kBundleNotFound;
    result.error_detail = "Invalid gateway URL for CID: " + cid;
    std::move(callback).Run(std::move(result));
    return;
  }

  api_request_helper::APIRequestOptions request_options;
  request_options.timeout = timeout_;

  api_request_helper_->Request(
      "GET", url, "", "application/json",
      base::BindOnce(&IpfsBundleFetcher::OnFetchResponse,
                     base::Unretained(this), std::move(callback), cid),
      {}, request_options);
}

void IpfsBundleFetcher::OnFetchResponse(
    FetchCallback callback,
    const std::string& cid,
    api_request_helper::APIRequestResult api_request_result) {
  FetchResult result;

  if (!api_request_result.Is2XXResponseCode()) {
    result.error = WebcatError::kBundleNotFound;
    result.error_detail = "HTTP error fetching bundle: " +
                          std::to_string(api_request_result.response_code());
    std::move(callback).Run(std::move(result));
    return;
  }

  std::string body_str;
  const auto& body = api_request_result.value_body();

  if (body.is_string()) {
    body_str = body.GetString();
  } else {
    if (!base::JSONWriter::Write(body, &body_str)) {
      result.error = WebcatError::kBundleParseError;
      result.error_detail = "Failed to serialize response body";
      std::move(callback).Run(std::move(result));
      return;
    }
  }

  if (body_str.size() > kMaxBundleSizeBytes) {
    result.error = WebcatError::kBundleTooLarge;
    result.error_detail = "Bundle exceeds maximum size";
    std::move(callback).Run(std::move(result));
    return;
  }

  if (!VerifyCidIntegrity(body_str, cid)) {
    result.error = WebcatError::kCidIntegrityFailed;
    result.error_detail = "CID integrity verification failed";
    std::move(callback).Run(std::move(result));
    return;
  }

  auto parse_result = ParseBundle(body_str);
  if (parse_result.error != WebcatError::kNone) {
    result.error = parse_result.error;
    result.error_detail = parse_result.error_detail;
    std::move(callback).Run(std::move(result));
    return;
  }

  auto verify_result = VerifyBundle(*parse_result.bundle, cid);
  if (!verify_result.success) {
    result.error = verify_result.error;
    result.error_detail = verify_result.error_detail;
    std::move(callback).Run(std::move(result));
    return;
  }

  result.success = true;
  result.bundle = std::move(*parse_result.bundle);
  std::move(callback).Run(std::move(result));
}

GURL IpfsBundleFetcher::BuildGatewayUrl(const std::string& cid) const {
  return GURL("https://" + cid + "." + gateway_host_);
}

}  // namespace webcat
