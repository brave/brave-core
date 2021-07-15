/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/brave_p3a_uploader.h"

#include <utility>

#include "base/base64.h"
#include "net/base/load_flags.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"

namespace brave {

namespace {

// TODO(iefremov): Provide more details for the traffic annotation.
net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotation(
    base::StringPiece upload_type) {
  if (upload_type == "p3a") {
    return net::DefineNetworkTrafficAnnotation("metrics_report_uma", R"(
        semantics {
          sender: "Brave Privacy-Preserving Product Analytics Uploader"
          description:
            "Report of anonymized usage statistics. For more info, see "
            "https://brave.com/P3A"
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
            "Users can enable or disable it in brave://settings/privacy"
           policy_exception_justification:
             "Not implemented."
        })");
  }
  DCHECK_EQ(upload_type, "p2a");
  return net::DefineNetworkTrafficAnnotation("metrics_report_uma", R"(
      semantics {
        sender: "Brave Privacy-Preserving Ad Analytics Uploader"
        description:
          "Report of anonymized usage statistics. For more info, see "
          "https://brave.com/P2A"
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
          "Users can enable or disable it by enabling or disabling Brave rewards
         or ads in brave://rewards"
         policy_exception_justification:
           "Not implemented."
      })");
}

}  // namespace

BraveP3AUploader::BraveP3AUploader(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    const GURL& p3a_endpoint,
    const GURL& p2a_endpoint,
    const UploadCallback& on_upload_complete)
    : url_loader_factory_(url_loader_factory),
      p3a_endpoint_(p3a_endpoint),
      p2a_endpoint_(p2a_endpoint),
      on_upload_complete_(on_upload_complete) {}

BraveP3AUploader::~BraveP3AUploader() = default;

void BraveP3AUploader::UploadLog(const std::string& compressed_log_data,
                                 const std::string& upload_type) {
  auto resource_request = std::make_unique<network::ResourceRequest>();
  if (upload_type == "p2a") {
    resource_request->url = p2a_endpoint_;
    resource_request->headers.SetHeader("X-Brave-P2A", "?1");
  } else if (upload_type == "p3a") {
    resource_request->url = p3a_endpoint_;
    resource_request->headers.SetHeader("X-Brave-P3A", "?1");
  } else {
    NOTREACHED();
  }

  resource_request->credentials_mode = network::mojom::CredentialsMode::kOmit;
  resource_request->method = "POST";

  url_loader_ = network::SimpleURLLoader::Create(
      std::move(resource_request),
      GetNetworkTrafficAnnotation(upload_type));
  std::string base64;
  base::Base64Encode(compressed_log_data, &base64);
  url_loader_->AttachStringForUpload(base64, "application/base64");

  url_loader_->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      url_loader_factory_.get(),
      base::BindOnce(&BraveP3AUploader::OnUploadComplete,
                     base::Unretained(this)));
}

void BraveP3AUploader::OnUploadComplete(
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
