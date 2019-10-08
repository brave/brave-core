/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CONTENT_BROWSER_WEBUI_BRAVE_SHARED_RESOURCES_DATA_SOURCE_H_
#define BRAVE_CONTENT_BROWSER_WEBUI_BRAVE_SHARED_RESOURCES_DATA_SOURCE_H_

#include <string>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/single_thread_task_runner.h"
#include "content/public/browser/url_data_source.h"

namespace brave_content {

// A DataSource for chrome://brave-resources/ URLs.
class BraveSharedResourcesDataSource : public content::URLDataSource {
 public:
  BraveSharedResourcesDataSource();
  ~BraveSharedResourcesDataSource() override;

  // URLDataSource implementation.
  std::string GetSource() override;
  void StartDataRequest(
      const std::string& path,
      const content::WebContents::Getter& wc_getter,
      const content::URLDataSource::GotDataCallback& callback) override;
  bool AllowCaching() override;
  std::string GetMimeType(const std::string& path) override;
  bool ShouldServeMimeTypeAsContentTypeHeader() override;
  scoped_refptr<base::SingleThreadTaskRunner> TaskRunnerForRequestPath(
      const std::string& path) override;
  std::string GetAccessControlAllowOriginForOrigin(
      const std::string& origin) override;
  bool IsGzipped(const std::string& path) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(BraveSharedResourcesDataSource);
};

}  // namespace brave_content

#endif  // BRAVE_CONTENT_BROWSER_WEBUI_BRAVE_SHARED_RESOURCES_DATA_SOURCE_H_
