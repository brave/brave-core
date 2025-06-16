/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/uploader.h"

#include <utility>

#include "base/check.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/p3a/constellation_helper.h"
#include "brave/components/p3a/metric_log_type.h"
#include "brave/components/p3a/network_annotations.h"
#include "brave/components/p3a/p3a_config.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"

namespace p3a {

namespace {

constexpr char kBraveP3AHeader[] = "X-Brave-P3A";
constexpr char kBraveP3AVersionHeader[] = "Brave-P3A-Version";
constexpr char kBraveP3AConstellationThresholdHeader[] =
    "Brave-P3A-Constellation-Threshold";

constexpr size_t kCurrentP3AVersionValue = 3;

GURL GetConstellationUploadURL(const P3AConfig* config,
                               MetricLogType log_type,
                               const std::string& upload_type) {
  std::string path;
  if (upload_type == kP3ACreativeUploadType) {
    path = "creative";
  } else {
    path = MetricLogTypeToString(log_type);
  }
  return GURL(base::StrCat({config->p3a_constellation_upload_host, "/", path}));
}

}  // namespace

Uploader::Uploader(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    UploadCompleteCallback upload_callback,
    const P3AConfig* config)
    : url_loader_factory_(url_loader_factory),
      config_(config),
      upload_callback_(upload_callback) {}

Uploader::~Uploader() = default;

void Uploader::UploadLog(const std::string& compressed_log_data,
                         const std::string& upload_type,
                         bool is_nebula,
                         MetricLogType log_type) {
  auto resource_request = std::make_unique<network::ResourceRequest>();
  resource_request->credentials_mode = network::mojom::CredentialsMode::kOmit;
  resource_request->method = "POST";

  resource_request->url =
      GetConstellationUploadURL(config_, log_type, upload_type);
  resource_request->headers.SetHeader(
      kBraveP3AVersionHeader, base::NumberToString(kCurrentP3AVersionValue));

  const size_t threshold =
      is_nebula ? kNebulaThreshold : kConstellationDefaultThreshold;
  resource_request->headers.SetHeader(kBraveP3AConstellationThresholdHeader,
                                      base::NumberToString(threshold));
  resource_request->headers.SetHeader(kBraveP3AHeader, "?1");

#if defined(OFFICIAL_BUILD)
  CHECK(!resource_request->url.is_empty() &&
        resource_request->url.SchemeIsHTTPOrHTTPS());
#else
  if (resource_request->url.is_empty()) {
    // If the upload URL is empty, ignore the request and act as if it
    // succeeded.
    upload_callback_.Run(true, 0, log_type);
    return;
  }
#endif

  constellation_url_loaders_[log_type] = network::SimpleURLLoader::Create(
      std::move(resource_request), GetP3AUploadAnnotation());
  network::SimpleURLLoader* url_loader =
      constellation_url_loaders_[log_type].get();

  url_loader->AttachStringForUpload(compressed_log_data, "text/plain");

  url_loader->DownloadHeadersOnly(
      url_loader_factory_.get(),
      base::BindOnce(&Uploader::OnUploadComplete, base::Unretained(this),
                     log_type));
}

void Uploader::OnUploadComplete(
    MetricLogType log_type,
    scoped_refptr<net::HttpResponseHeaders> headers) {
  int response_code = -1;
  network::SimpleURLLoader* url_loader =
      constellation_url_loaders_[log_type].get();

  if (headers) {
    response_code = headers->response_code();
  }
  bool is_ok = url_loader->NetError() == net::OK;
  constellation_url_loaders_.erase(log_type);
  upload_callback_.Run(is_ok, response_code, log_type);
}

}  // namespace p3a
