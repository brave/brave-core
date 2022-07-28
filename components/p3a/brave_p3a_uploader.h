/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_P3A_BRAVE_P3A_UPLOADER_H_
#define BRAVE_COMPONENTS_P3A_BRAVE_P3A_UPLOADER_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/containers/flat_map.h"
#include "base/memory/ref_counted.h"
#include "brave/components/p3a/metric_log_type.h"

namespace network {
class SharedURLLoaderFactory;
class SimpleURLLoader;
}  // namespace network

namespace net {
class HttpResponseHeaders;
}  // namespace net

namespace brave {

constexpr char kP2AUploadType[] = "p2a";
constexpr char kP3AUploadType[] = "p3a";
constexpr char kP3ACreativeUploadType[] = "p3a_creative";

struct BraveP3AConfig;

class BraveP3AUploader {
 public:
  using UploadCompleteCallback =
      base::RepeatingCallback<void(bool is_ok,
                                   int response_code,
                                   bool is_star,
                                   MetricLogType log_type)>;

  BraveP3AUploader(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      UploadCompleteCallback upload_callback,
      BraveP3AConfig* config);

  BraveP3AUploader(const BraveP3AUploader&) = delete;
  BraveP3AUploader& operator=(const BraveP3AUploader&) = delete;

  ~BraveP3AUploader();

  // From metrics::MetricsLogUploader
  void UploadLog(const std::string& compressed_log_data,
                 const std::string& upload_type,
                 bool is_star,
                 MetricLogType log_type);

  void OnUploadComplete(bool is_star,
                        MetricLogType log_type,
                        scoped_refptr<net::HttpResponseHeaders> headers);

 private:
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  base::flat_map<MetricLogType, std::unique_ptr<network::SimpleURLLoader>>
      url_loaders_;

  BraveP3AConfig* config_;

  UploadCompleteCallback upload_callback_;

  std::unique_ptr<network::SimpleURLLoader> url_loader_;
};

}  // namespace brave

#endif  // BRAVE_COMPONENTS_P3A_BRAVE_P3A_UPLOADER_H_
