/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_P3A_UPLOADER_H_
#define BRAVE_COMPONENTS_P3A_UPLOADER_H_

#include <memory>
#include <string>

#include "base/containers/flat_map.h"
#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/ref_counted.h"
#include "brave/components/p3a/metric_log_type.h"

namespace network {
class SharedURLLoaderFactory;
class SimpleURLLoader;
}  // namespace network

namespace net {
class HttpResponseHeaders;
}  // namespace net

namespace p3a {

inline constexpr char kP2AUploadType[] = "p2a";
inline constexpr char kP3AUploadType[] = "p3a";
inline constexpr char kP3ACreativeUploadType[] = "p3a_creative";

struct P3AConfig;

// Handles uploading of JSON and Constellation metrics to Brave servers.
// The endpoint used may differ depending on whether the measurement
// is P3A, P2A, NTP-SI P3A as well as whether it is in JSON or Constellation
// format.
class Uploader {
 public:
  using UploadCompleteCallback =
      base::RepeatingCallback<void(bool is_ok,
                                   int response_code,
                                   bool is_constellation,
                                   MetricLogType log_type)>;

  Uploader(scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
           UploadCompleteCallback upload_callback,
           const P3AConfig* config);

  Uploader(const Uploader&) = delete;
  Uploader& operator=(const Uploader&) = delete;

  ~Uploader();

  // From metrics::MetricsLogUploader
  void UploadLog(const std::string& compressed_log_data,
                 const std::string& upload_type,
                 bool is_constellation,
                 MetricLogType log_type);

  void OnUploadComplete(bool is_constellation,
                        MetricLogType log_type,
                        scoped_refptr<net::HttpResponseHeaders> headers);

 private:
  base::flat_map<MetricLogType, std::unique_ptr<network::SimpleURLLoader>>&
  GetURLLoaders(bool is_constellation);

  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  base::flat_map<MetricLogType, std::unique_ptr<network::SimpleURLLoader>>
      json_url_loaders_;
  base::flat_map<MetricLogType, std::unique_ptr<network::SimpleURLLoader>>
      constellation_url_loaders_;

  const raw_ptr<const P3AConfig> config_;

  UploadCompleteCallback upload_callback_;
};

}  // namespace p3a

#endif  // BRAVE_COMPONENTS_P3A_UPLOADER_H_
