/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_P3A_BRAVE_P3A_UPLOADER_H_
#define BRAVE_COMPONENTS_P3A_BRAVE_P3A_UPLOADER_H_

#include <memory>
#include <string>

#include "base/containers/flat_map.h"
#include "base/functional/callback.h"
#include "base/memory/ref_counted.h"
#include "brave/components/p3a/metric_log_type.h"
#include "url/gurl.h"

namespace network {
class SharedURLLoaderFactory;
class SimpleURLLoader;
}  // namespace network

namespace brave {

constexpr char kP2AUploadType[] = "p2a";
constexpr char kP3AUploadType[] = "p3a";
constexpr char kP3ACreativeUploadType[] = "p3a_creative";

// Handle uploading logged metrics to the correct endpoints.
class BraveP3AUploader {
 public:
  using UploadCallback = base::RepeatingCallback<void(int response_code,
                                                      int error_code,
                                                      bool was_https,
                                                      MetricLogType log_type)>;

  BraveP3AUploader(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      const GURL& p3a_endpoint,
      const GURL& p3a_creative_endpoint,
      const GURL& p2a_endpoint,
      const UploadCallback& on_upload_complete);

  BraveP3AUploader(const BraveP3AUploader&) = delete;
  BraveP3AUploader& operator=(const BraveP3AUploader&) = delete;

  ~BraveP3AUploader();

  // From metrics::MetricsLogUploader
  void UploadLog(const std::string& compressed_log_data,
                 const std::string& upload_type,
                 MetricLogType log_type);

  void OnUploadComplete(MetricLogType log_type,
                        std::unique_ptr<std::string> response_body);

 private:
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  const GURL p3a_endpoint_;
  const GURL p3a_creative_endpoint_;
  const GURL p2a_endpoint_;
  const UploadCallback on_upload_complete_;
  base::flat_map<MetricLogType, std::unique_ptr<network::SimpleURLLoader>>
      url_loaders_;
};

}  // namespace brave

#endif  // BRAVE_COMPONENTS_P3A_BRAVE_P3A_UPLOADER_H_
