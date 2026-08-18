/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/services/network/simple_url_loader_service.h"

#include <algorithm>
#include <string>
#include <string_view>
#include <utility>

#include "base/containers/span.h"
#include "base/functional/bind.h"
#include "net/base/net_errors.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_util.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "url/gurl.h"

namespace brave::network {

namespace {

std::string_view MethodToString(mojom::HttpMethod method) {
  switch (method) {
    case mojom::HttpMethod::kGet:
      return net::HttpRequestHeaders::kGetMethod;
    case mojom::HttpMethod::kHead:
      return net::HttpRequestHeaders::kHeadMethod;
    case mojom::HttpMethod::kPost:
      return net::HttpRequestHeaders::kPostMethod;
    case mojom::HttpMethod::kPut:
      return net::HttpRequestHeaders::kPutMethod;
    case mojom::HttpMethod::kPatch:
      return net::HttpRequestHeaders::kPatchMethod;
    case mojom::HttpMethod::kDelete:
      return net::HttpRequestHeaders::kDeleteMethod;
  }
}

std::string_view AsStringView(const std::vector<uint8_t>& bytes) {
  return std::string_view(reinterpret_cast<const char*>(bytes.data()),
                          bytes.size());
}

std::vector<mojom::HttpHeaderPtr> CollectResponseHeaders(
    const net::HttpResponseHeaders* headers) {
  std::vector<mojom::HttpHeaderPtr> result;
  if (!headers) {
    return result;
  }

  size_t iter = 0;
  std::string name;
  std::string value;
  while (headers->EnumerateHeaderLines(&iter, &name, &value)) {
    auto header = mojom::HttpHeader::New();
    header->name = name;
    // Header values are arbitrary octets, which is why the Mojom carries them
    // as array<uint8> rather than string. Sending a non-UTF-8 Mojom string
    // fails validation and terminates the sending process, so this must not
    // become a string without sanitizing first.
    header->value = std::vector<uint8_t>(value.begin(), value.end());
    result.push_back(std::move(header));
  }
  return result;
}

mojom::DownloadResultPtr MakeErrorResult(int net_error) {
  auto result = mojom::DownloadResult::New();
  result->net_error = net_error;
  result->response_code = -1;
  return result;
}

}  // namespace

SimpleUrlLoaderService::SimpleUrlLoaderService(
    scoped_refptr<::network::SharedURLLoaderFactory> url_loader_factory,
    net::NetworkTrafficAnnotationTag traffic_annotation)
    : url_loader_factory_(std::move(url_loader_factory)),
      traffic_annotation_(traffic_annotation) {
  CHECK(url_loader_factory_);
}

SimpleUrlLoaderService::~SimpleUrlLoaderService() = default;

void SimpleUrlLoaderService::Bind(
    mojo::PendingReceiver<mojom::SimpleUrlLoader> receiver) {
  receivers_.Add(this, std::move(receiver));
}

void SimpleUrlLoaderService::Download(mojom::DownloadRequestPtr request,
                                      DownloadCallback callback) {
  GURL url(request->url);
  if (!url.is_valid() || !url.SchemeIsHTTPOrHTTPS()) {
    std::move(callback).Run(MakeErrorResult(net::ERR_INVALID_URL));
    return;
  }

  auto resource_request = std::make_unique<::network::ResourceRequest>();
  resource_request->url = std::move(url);
  resource_request->method = std::string(MethodToString(request->method));

  for (const auto& header : request->headers) {
    // SetHeader DCHECKs on malformed input, and these values crossed a process
    // boundary from a non-C++ caller, so validate rather than trust.
    std::string_view value = AsStringView(header->value);
    if (!net::HttpUtil::IsValidHeaderName(header->name) ||
        !net::HttpUtil::IsValidHeaderValue(value)) {
      std::move(callback).Run(MakeErrorResult(net::ERR_INVALID_ARGUMENT));
      return;
    }
    resource_request->headers.SetHeader(header->name, value);
  }

  const bool has_body = request->body.has_value();
  if (has_body != request->body_content_type.has_value()) {
    std::move(callback).Run(MakeErrorResult(net::ERR_INVALID_ARGUMENT));
    return;
  }

  auto loader = ::network::SimpleURLLoader::Create(std::move(resource_request),
                                                   traffic_annotation_);
  loader->SetAllowHttpErrorResults(request->allow_http_error_results);

  if (has_body) {
    loader->AttachStringForUpload(AsStringView(*request->body),
                                  *request->body_content_type);
  }

  size_t max_response_bytes =
      ::network::SimpleURLLoader::kMaxBoundedStringDownloadSize;
  if (request->max_response_bytes > 0) {
    max_response_bytes = std::min(
        max_response_bytes, static_cast<size_t>(request->max_response_bytes));
  }

  ::network::SimpleURLLoader* loader_ptr = loader.get();
  loaders_.insert(std::move(loader));

  loader_ptr->DownloadToString(
      url_loader_factory_.get(),
      base::BindOnce(&SimpleUrlLoaderService::OnDownloadComplete,
                     weak_factory_.GetWeakPtr(), loader_ptr,
                     std::move(callback)),
      max_response_bytes);
}

void SimpleUrlLoaderService::OnDownloadComplete(
    ::network::SimpleURLLoader* loader,
    DownloadCallback callback,
    std::optional<std::string> body) {
  auto result = mojom::DownloadResult::New();
  result->net_error = loader->NetError();
  result->response_code = -1;
  result->final_url = loader->GetFinalURL().spec();

  const auto* response_info = loader->ResponseInfo();
  if (response_info && response_info->headers) {
    result->response_code = response_info->headers->response_code();
    result->headers = CollectResponseHeaders(response_info->headers.get());
  }

  if (body) {
    result->body = std::vector<uint8_t>(body->begin(), body->end());
  }

  // Erasing the loader destroys it, so everything read from it must already
  // have been copied out above. base::UniquePtrComparator makes `find`
  // heterogeneous, but `erase` still needs an iterator.
  auto it = loaders_.find(loader);
  CHECK(it != loaders_.end());
  loaders_.erase(it);

  std::move(callback).Run(std::move(result));
}

}  // namespace brave::network
