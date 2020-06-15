/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_P2A_BRAVE_P2A_UPLOADER_H_
#define BRAVE_COMPONENTS_P2A_BRAVE_P2A_UPLOADER_H_

#include <memory>
#include <string>

#include "components/metrics/metrics_log_uploader.h"
#include "url/gurl.h"

namespace network {
class SharedURLLoaderFactory;
class SimpleURLLoader;
}  // namespace network

namespace brave {

class BraveP2AUploader : public metrics::MetricsLogUploader {
 public:
  BraveP2AUploader(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      const GURL server_url,
      const MetricsLogUploader::UploadCallback& on_upload_complete);

  ~BraveP2AUploader() override;

  // From metrics::MetricsLogUploader
  void UploadLog(const std::string& compressed_log_data,
                 const std::string& log_hash,
                 const std::string& log_signature,
                 const metrics::ReportingInfo& reporting_info) override;

  void OnUploadComplete(std::unique_ptr<std::string> response_body);

 private:
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  const GURL server_url_;
  const MetricsLogUploader::UploadCallback on_upload_complete_;
  std::unique_ptr<network::SimpleURLLoader> url_loader_;
  DISALLOW_COPY_AND_ASSIGN(BraveP2AUploader);
};

}  // namespace brave

#endif  // BRAVE_COMPONENTS_P2A_BRAVE_P2A_UPLOADER_H_
