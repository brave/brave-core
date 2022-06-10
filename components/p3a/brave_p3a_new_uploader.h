/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_P3A_BRAVE_P3A_NEW_UPLOADER_H_
#define BRAVE_COMPONENTS_P3A_BRAVE_P3A_NEW_UPLOADER_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "url/gurl.h"

namespace network {
class SharedURLLoaderFactory;
class SimpleURLLoader;
}  // namespace network

namespace net {
class HttpResponseHeaders;
}  // namespace net

namespace brave {

// This will replace the "normal" uploader when the server-side is ready.
// The difference is only in endpoint, mime and lack of base64 encoding.
// Also this one doesn't report the upload status back (since this one is for
// testing the new endpoint).
class BraveP3ANewUploader {
 public:
  using UploadCompleteCallback = base::RepeatingCallback<
      void(bool is_ok, int response_code, bool is_star)>;

  BraveP3ANewUploader(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      UploadCompleteCallback upload_callback,
      const GURL& p3a_json_endpoint,
      const GURL& p2a_json_endpoint,
      const GURL& p3a_star_endpoint,
      const GURL& p2a_star_endpoint);

  BraveP3ANewUploader(const BraveP3ANewUploader&) = delete;
  BraveP3ANewUploader& operator=(const BraveP3ANewUploader&) = delete;

  ~BraveP3ANewUploader();

  // From metrics::MetricsLogUploader
  void UploadLog(const std::string& compressed_log_data,
                 const std::string& log_type,
                 bool is_star);

  void OnUploadComplete(bool is_star,
                        scoped_refptr<net::HttpResponseHeaders> headers);

 private:
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;

  const GURL p3a_json_endpoint_;
  const GURL p2a_json_endpoint_;
  const GURL p3a_star_endpoint_;
  const GURL p2a_star_endpoint_;

  UploadCompleteCallback upload_callback_;

  std::unique_ptr<network::SimpleURLLoader> url_loader_;
};

}  // namespace brave

#endif  // BRAVE_COMPONENTS_P3A_BRAVE_P3A_NEW_UPLOADER_H_
