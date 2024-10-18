/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_WEBCOMPAT_REPORTER_BROWSER_WEBCOMPAT_REPORT_UPLOADER_H_
#define BRAVE_COMPONENTS_WEBCOMPAT_REPORTER_BROWSER_WEBCOMPAT_REPORT_UPLOADER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/memory/scoped_refptr.h"
#include "base/sequence_checker.h"
#include "base/values.h"
#include "brave/components/webcompat_reporter/common/webcompat_reporter.mojom.h"
#include "url/gurl.h"

namespace network {
class SharedURLLoaderFactory;
class SimpleURLLoader;
}  // namespace network

namespace webcompat_reporter {

class WebcompatReportUploader {
 public:
  explicit WebcompatReportUploader(
      scoped_refptr<network::SharedURLLoaderFactory>);
  WebcompatReportUploader(const WebcompatReportUploader&) = delete;
  WebcompatReportUploader& operator=(const WebcompatReportUploader&) = delete;
  virtual ~WebcompatReportUploader();

  virtual void SubmitReport(mojom::ReportInfoPtr report_info);

 private:
  std::unique_ptr<network::SimpleURLLoader> simple_url_loader_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  void CreateAndStartURLLoader(const GURL& upload_url,
                               const std::string& content_type,
                               const std::string& post_data);
  void OnSimpleURLLoaderComplete(std::unique_ptr<std::string> response_body);
  SEQUENCE_CHECKER(sequence_checker_);
};

}  // namespace webcompat_reporter

#endif  // BRAVE_COMPONENTS_WEBCOMPAT_REPORTER_BROWSER_WEBCOMPAT_REPORT_UPLOADER_H_
