/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/brave_p3a_uploader.h"

#include <utility>

#include "brave/components/p3a/brave_p3a_config.h"
#include "brave/components/p3a/network_annotations.h"
#include "net/base/load_flags.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/url_response_head.mojom.h"

namespace brave {

BraveP3AUploader::BraveP3AUploader(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    UploadCompleteCallback upload_callback,
    BraveP3AConfig* config)
    : url_loader_factory_(url_loader_factory),
      config_(config),
      upload_callback_(upload_callback) {}

BraveP3AUploader::~BraveP3AUploader() = default;

void BraveP3AUploader::UploadLog(const std::string& compressed_log_data,
                                 const std::string& upload_type,
                                 bool is_star,
                                 MetricLogType log_type) {
  auto resource_request = std::make_unique<network::ResourceRequest>();
  if (upload_type == kP2AUploadType) {
    resource_request->url =
        is_star ? config_->p2a_star_upload_url : config_->p2a_json_upload_url;
    resource_request->headers.SetHeader("X-Brave-P2A", "?1");
  } else if (upload_type == kP3AUploadType) {
    resource_request->url =
        is_star ? config_->p3a_star_upload_url : config_->p3a_json_upload_url;
    resource_request->headers.SetHeader("X-Brave-P3A", "?1");
  } else if (upload_type == kP3ACreativeUploadType) {
    resource_request->url = config_->p3a_creative_upload_url;
    resource_request->headers.SetHeader("X-Brave-P3A", "?1");
  } else {
    NOTREACHED();
  }

  resource_request->credentials_mode = network::mojom::CredentialsMode::kOmit;
  resource_request->method = "POST";

  url_loaders_[log_type] = network::SimpleURLLoader::Create(
      std::move(resource_request),
      GetP3AUploadAnnotation(upload_type, is_star));
  network::SimpleURLLoader* url_loader = url_loaders_[log_type].get();

  url_loader->AttachStringForUpload(
      compressed_log_data, is_star ? "text/plain" : "application/json");

  url_loader->DownloadHeadersOnly(
      url_loader_factory_.get(),
      base::BindOnce(&BraveP3AUploader::OnUploadComplete,
                     base::Unretained(this), is_star, log_type));
}

void BraveP3AUploader::OnUploadComplete(
    bool is_star,
    MetricLogType log_type,
    scoped_refptr<net::HttpResponseHeaders> headers) {
  int response_code = -1;
  network::SimpleURLLoader* url_loader = url_loaders_[log_type].get();

  if (headers) {
    response_code = headers->response_code();
  }
  bool is_ok = url_loader->NetError() == net::OK;
  url_loaders_.erase(log_type);
  upload_callback_.Run(is_ok, response_code, is_star, log_type);
}

}  // namespace brave
