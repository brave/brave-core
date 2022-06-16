/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/brave_p3a_new_uploader.h"

#include <utility>

#include "brave/components/p3a/network_annotations.h"
#include "net/base/load_flags.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/url_response_head.mojom.h"

namespace brave {

BraveP3ANewUploader::BraveP3ANewUploader(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    UploadCompleteCallback upload_callback,
    const GURL& p3a_json_endpoint,
    const GURL& p2a_json_endpoint,
    const GURL& p3a_star_endpoint,
    const GURL& p2a_star_endpoint)
    : url_loader_factory_(url_loader_factory),
      p3a_json_endpoint_(p3a_json_endpoint),
      p2a_json_endpoint_(p2a_json_endpoint),
      p3a_star_endpoint_(p3a_star_endpoint),
      p2a_star_endpoint_(p2a_star_endpoint),
      upload_callback_(upload_callback) {}

BraveP3ANewUploader::~BraveP3ANewUploader() = default;

void BraveP3ANewUploader::UploadLog(const std::string& compressed_log_data,
                                    const std::string& log_type,
                                    bool is_star) {
  auto resource_request = std::make_unique<network::ResourceRequest>();
  if (log_type == "p2a") {
    resource_request->url = is_star ? p2a_star_endpoint_ : p2a_json_endpoint_;
    resource_request->headers.SetHeader("X-Brave-P2A", "?1");
  } else if (log_type == "p3a") {
    resource_request->url = is_star ? p3a_star_endpoint_ : p3a_json_endpoint_;
    resource_request->headers.SetHeader("X-Brave-P3A", "?1");
  } else {
    NOTREACHED();
  }

  resource_request->credentials_mode = network::mojom::CredentialsMode::kOmit;
  resource_request->method = "POST";

  url_loader_ = network::SimpleURLLoader::Create(
      std::move(resource_request), GetP3AUploadAnnotation(log_type, is_star));
  url_loader_->AttachStringForUpload(
      compressed_log_data, is_star ? "text/plain" : "application/json");

  url_loader_->DownloadHeadersOnly(
      url_loader_factory_.get(),
      base::BindOnce(&BraveP3ANewUploader::OnUploadComplete,
                     base::Unretained(this), is_star));
}

void BraveP3ANewUploader::OnUploadComplete(
    bool is_star,
    scoped_refptr<net::HttpResponseHeaders> headers) {
  int response_code = -1;
  if (headers) {
    response_code = headers->response_code();
  }
  bool is_ok = url_loader_->NetError() != net::OK || response_code < 200 ||
               response_code > 299;
  url_loader_.reset();
  upload_callback_.Run(is_ok, response_code, is_star);
}

}  // namespace brave
