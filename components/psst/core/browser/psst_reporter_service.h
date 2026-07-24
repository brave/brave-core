// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_PSST_CORE_BROWSER_PSST_REPORTER_SERVICE_H_
#define BRAVE_COMPONENTS_PSST_CORE_BROWSER_PSST_REPORTER_SERVICE_H_

#include <memory>

#include "base/containers/flat_set.h"
#include "base/functional/callback.h"
#include "brave/components/psst/core/common/psst_script_responses.h"
#include "components/keyed_service/core/keyed_service.h"

namespace psst {
class PsstErrorReportUploader;

// Orders PolicyTasks by uid so they can be deduplicated in a flat_set.
struct PolicyTaskCompare {
  bool operator()(const PolicyTask& lhs, const PolicyTask& rhs) const;
};
using PolicyTasksSet = base::flat_set<PolicyTask, PolicyTaskCompare>;
using OnSubmitPsstErrorsReportCallback = base::OnceCallback<void()>;

// Returns the installed PSST component version, or nullopt if not found.
using ComponentVersionCallback =
    base::RepeatingCallback<std::optional<std::string>()>;

// Returns the current release channel name (e.g. "stable", "beta").
using ChannelNameCallback = base::RepeatingCallback<std::string()>;

class PsstReporterService : public KeyedService {
 public:
  explicit PsstReporterService(
      ChannelNameCallback channel_name_callback,
      ComponentVersionCallback component_version_callback,
      std::unique_ptr<PsstErrorReportUploader> report_uploader);
  ~PsstReporterService() override;

  void SubmitPsstErrorsReport(std::optional<PolicyTasksSet> failed_policy_tasks,
                              const int script_version,
                              OnSubmitPsstErrorsReportCallback callback);

 private:
  ChannelNameCallback channel_name_callback_;
  ComponentVersionCallback component_version_callback_;
  std::unique_ptr<PsstErrorReportUploader> report_uploader_;
};

}  //  namespace psst

#endif  // BRAVE_COMPONENTS_PSST_CORE_BROWSER_PSST_REPORTER_SERVICE_H_
