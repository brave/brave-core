// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_PSST_CORE_BROWSER_PSST_REPORT_UPLOADER_H_
#define BRAVE_COMPONENTS_PSST_CORE_BROWSER_PSST_REPORT_UPLOADER_H_

#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"

namespace psst {

class PsstErrorReportUploader {
 public:
  explicit PsstErrorReportUploader(
      scoped_refptr<network::SharedURLLoaderFactory>);
  PsstErrorReportUploader(const PsstErrorReportUploader&) = delete;
  PsstErrorReportUploader& operator=(const PsstErrorReportUploader&) = delete;

  ~PsstErrorReportUploader();

  void Upload();

 private:
  std::unique_ptr<network::SimpleURLLoader> simple_url_loader_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  void CreateAndStartURLLoader(const GURL& upload_url,
                               const std::string& content_type,
                               const std::string& post_data);
  void OnSimpleURLLoaderComplete(std::optional<std::string> response_body);
  SEQUENCE_CHECKER(sequence_checker_);
};

}  // namespace psst

#endif  // BRAVE_COMPONENTS_PSST_CORE_BROWSER_PSST_REPORT_UPLOADER_H_
