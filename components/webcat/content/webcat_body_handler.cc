/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/webcat/content/webcat_body_handler.h"

#include <optional>
#include <string>
#include <utility>

#include "base/base64url.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "crypto/sha2.h"
#include "net/http/http_response_headers.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "services/network/public/cpp/resource_request.h"

namespace webcat {

namespace {

std::string Base64UrlEncodeHash(base::span<const uint8_t> data) {
  std::string encoded;
  base::Base64UrlEncode(base::as_chars(data),
                        base::Base64UrlEncodePolicy::OMIT_PADDING, &encoded);
  return encoded;
}

bool IsHashableResourceType(const std::string& mime_type) {
  return base::StartsWith(mime_type, "text/html") ||
         base::StartsWith(mime_type, "text/css") ||
         base::StartsWith(mime_type, "application/javascript") ||
         base::StartsWith(mime_type, "application/wasm") ||
         base::StartsWith(mime_type, "text/javascript");
}

}  // namespace

WebcatBodyHandler::WebcatBodyHandler(const Manifest& manifest)
    : manifest_(manifest) {}

WebcatBodyHandler::~WebcatBodyHandler() = default;

bool WebcatBodyHandler::OnRequest(network::ResourceRequest* request) {
  if (!request) {
    return false;
  }

  response_url_ = request->url;
  is_main_frame_ = request->is_main_frame;
  return true;
}

bool WebcatBodyHandler::ShouldProcess(const GURL& response_url,
                                      network::mojom::URLResponseHead* response_head,
                                      bool* defer) {
  if (!response_head || !response_head->headers) {
    return false;
  }

  std::string mime_type = response_head->mime_type;
  if (!IsHashableResourceType(mime_type) && !is_main_frame_) {
    return false;
  }

  response_url_ = response_url;
  should_process_ = true;
  *defer = true;
  return true;
}

void WebcatBodyHandler::OnBeforeSending() {}

void WebcatBodyHandler::OnComplete() {}

body_sniffer::BodyHandler::Action WebcatBodyHandler::OnBodyUpdated(
    const std::string& body,
    bool is_complete) {
  if (!should_process_) {
    return Action::kComplete;
  }

  accumulated_body_.append(body);

  if (!is_complete) {
    return Action::kContinue;
  }

  std::string path = NormalizePath(response_url_.path());
  std::string expected_hash = GetExpectedHash(path);

  if (expected_hash.empty()) {
    return Action::kComplete;
  }

  auto result = VerifyContentHash(accumulated_body_, expected_hash);
  if (!result.success) {
    LOG(WARNING) << "Webcat: content integrity check failed for "
                 << response_url_.spec() << ": " << result.error_detail;
    return Action::kCancel;
  }

  return Action::kComplete;
}

bool WebcatBodyHandler::IsTransformer() const {
  return false;
}

void WebcatBodyHandler::Transform(
    std::string body,
    base::OnceCallback<void(std::string)> on_complete) {
  std::move(on_complete).Run(std::move(body));
}

void WebcatBodyHandler::UpdateResponseHead(
    network::mojom::URLResponseHead* response_head) {
  if (!response_head || !response_head->headers) {
    return;
  }

  std::string path = NormalizePath(response_url_.path());
  std::string csp = GetEffectiveCspForPath(path, manifest_.default_csp,
                                            manifest_.extra_csp);

  if (!csp.empty()) {
    net::HttpResponseHeaders* headers = response_head->headers.get();
    headers->RemoveHeader("Content-Security-Policy");
    headers->RemoveHeader("Content-Security-Policy-Report-Only");
    headers->AddHeader("Content-Security-Policy", csp);
  }
}

std::string WebcatBodyHandler::GetExpectedHash(
    const std::string& path) const {
  auto it = manifest_.files.find(path);
  if (it != manifest_.files.end()) {
    return it->second;
  }
  return std::string();
}

std::string WebcatBodyHandler::NormalizePath(const std::string& path) const {
  if (path == "/" || path.empty()) {
    return manifest_.default_index;
  }
  if (path.ends_with("/")) {
    std::string with_index = path + manifest_.default_index.substr(1);
    if (manifest_.files.contains(with_index)) {
      return with_index;
    }
  }
  if (!manifest_.files.contains(path)) {
    return manifest_.default_fallback;
  }
  return path;
}

}  // namespace webcat