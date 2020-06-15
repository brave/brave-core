/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/p2a/brave_p2a_uploader.h"

#include <utility>

#include "base/base64.h"
#include "net/base/load_flags.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"

namespace brave {

namespace {

// TODO(iefremov): Provide more details for the traffic annotation.
net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotation() {
  return net::DefineNetworkTrafficAnnotation("metrics_report_uma", R"(
      semantics {
        sender: "Brave Privacy-Preserving Ad Analytics Uploader"
        description:
          "Report of anonymized ad statistics. For more info, see https://brave.com/P2A"
        trigger:
          "Reports are automatically generated on startup and at intervals "
          "while Brave is running."
        data:
          "A protocol buffer with anonymized and encrypted usage data."
        destination: WEBSITE
      }
      policy {
        cookies_allowed: NO
        setting:
          "Users can enable or disable it by enabling/disabling ads in brave://rewards"
         policy_exception_justification:
           "Not implemented."
      })");
}

}  // namespace

BraveP2AUploader::BraveP2AUploader(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    const GURL server_url,
    const MetricsLogUploader::UploadCallback& on_upload_complete)
    : url_loader_factory_(url_loader_factory),
      server_url_(server_url),
      on_upload_complete_(on_upload_complete) {}

BraveP2AUploader::~BraveP2AUploader() = default;

void BraveP2AUploader::UploadLog(const std::string& compressed_log_data,
                                 const std::string& log_hash,
                                 const std::string& log_signature,
                                 const metrics::ReportingInfo& reporting_info) {
  auto resource_request = std::make_unique<network::ResourceRequest>();
  resource_request->url = server_url_;
  resource_request->credentials_mode = network::mojom::CredentialsMode::kOmit;
  resource_request->method = "POST";
  resource_request->headers.SetHeader("X-Brave-P2A", "?1");

  url_loader_ = network::SimpleURLLoader::Create(std::move(resource_request),
                                                 GetNetworkTrafficAnnotation());
  std::string base64;
  base::Base64Encode(compressed_log_data, &base64);
  url_loader_->AttachStringForUpload(base64, "application/base64");

  url_loader_->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      url_loader_factory_.get(),
      base::BindOnce(&BraveP2AUploader::OnUploadComplete,
                     base::Unretained(this)));
}

void BraveP2AUploader::OnUploadComplete(
    std::unique_ptr<std::string> response_body) {
  int response_code = -1;
  if (url_loader_->ResponseInfo() && url_loader_->ResponseInfo()->headers)
    response_code = url_loader_->ResponseInfo()->headers->response_code();

  int error_code = url_loader_->NetError();

  bool was_https = url_loader_->GetFinalURL().SchemeIs(url::kHttpsScheme);
  url_loader_.reset();
  on_upload_complete_.Run(response_code, error_code, was_https);
}

}  // namespace brave
