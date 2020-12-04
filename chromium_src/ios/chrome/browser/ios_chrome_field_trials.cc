/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ios/chrome/browser/ios_chrome_field_trials.h"

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/task/post_task.h"
#include "base/task/thread_pool.h"
#include "components/metrics/persistent_histograms.h"
#include "ios/chrome/browser/chrome_paths.h"

namespace {

void DeleteFileMetrics() {
  base::FilePath user_data_dir;
  if (base::PathService::Get(ios::DIR_USER_DATA, &user_data_dir)) {
    base::FilePath browser_metrics_upload_dir =
        user_data_dir.AppendASCII(kBrowserMetricsName);
      // When metrics reporting is not enabled, any existing files should be
      // deleted in order to preserve user privacy.
      base::ThreadPool::PostTask(
          FROM_HERE,
          {base::MayBlock(), base::TaskPriority::BEST_EFFORT,
           base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN},
          base::BindOnce(base::GetDeletePathRecursivelyCallback(),
                         std::move(browser_metrics_upload_dir)));
    }
}

}  // namespace

void IOSChromeFieldTrials::SetupFieldTrials() {
  DeleteFileMetrics();
}

void IOSChromeFieldTrials::SetupFeatureControllingFieldTrials(
    bool has_seed,
    base::FeatureList* feature_list) {
  // Add code here to enable field trials that are active at first run.
  // See http://crrev/c/1128269 for an example.
}
