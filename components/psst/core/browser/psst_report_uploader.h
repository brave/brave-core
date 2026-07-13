// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_PSST_CORE_BROWSER_PSST_REPORT_UPLOADER_H_
#define BRAVE_COMPONENTS_PSST_CORE_BROWSER_PSST_REPORT_UPLOADER_H_

#include <string>

#include "base/functional/callback_forward.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "base/values.h"
#include "url/gurl.h"

namespace network {
class SharedURLLoaderFactory;
class SimpleURLLoader;
}  // namespace network

namespace psst {

class PsstErrorReportUploader {
 public:
  explicit PsstErrorReportUploader(
      scoped_refptr<network::SharedURLLoaderFactory> factory);
  PsstErrorReportUploader(const PsstErrorReportUploader&) = delete;
  PsstErrorReportUploader& operator=(const PsstErrorReportUploader&) = delete;

  virtual ~PsstErrorReportUploader();

  virtual void Upload(std::optional<std::string> psst_component_version,
                      const int script_version,
                      const std::string& brave_version,
                      std::optional<std::string> channel,
                      base::ListValue failed_tasks,
                      base::OnceCallback<void()> callback);

 private:
  void CreateAndStartURLLoader(const GURL& upload_url,
                               const std::string& content_type,
                               const std::string& post_data,
                               base::OnceCallback<void()> callback);
  void OnSimpleURLLoaderComplete(base::OnceCallback<void()> callback,
                                 std::optional<std::string> response_body);
  std::unique_ptr<network::SimpleURLLoader> simple_url_loader_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  SEQUENCE_CHECKER(sequence_checker_);
  base::WeakPtrFactory<PsstErrorReportUploader> weak_ptr_factory_{this};
};

}  // namespace psst

#endif  // BRAVE_COMPONENTS_PSST_CORE_BROWSER_PSST_REPORT_UPLOADER_H_
