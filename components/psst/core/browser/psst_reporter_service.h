// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_PSST_CORE_BROWSER_PSST_REPORTER_SERVICE_H_
#define BRAVE_COMPONENTS_PSST_CORE_BROWSER_PSST_REPORTER_SERVICE_H_

#include <memory>

#include "base/containers/flat_set.h"
#include "base/functional/callback_forward.h"
#include "base/memory/raw_ptr.h"
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

class PsstReporterService : public KeyedService {
 public:
  class Delegate {
   public:
    struct ComponentInfo {
      std::string id;
      std::string name;
      std::string version;
    };
    virtual ~Delegate() = default;

    virtual std::vector<ComponentInfo> GetComponentInfos() const = 0;
    virtual std::string GetChannelName() const = 0;
  };
  using PsstComponentInfo = Delegate::ComponentInfo;

  explicit PsstReporterService(
      std::unique_ptr<Delegate> service_delegate,
      std::unique_ptr<PsstErrorReportUploader> report_uploader);
  ~PsstReporterService() override;

  void SubmitPsstErrorsReport(std::optional<PolicyTasksSet> failed_policy_tasks,
                              const int script_version,
                              OnSubmitPsstErrorsReportCallback callback);

 private:
  std::unique_ptr<Delegate> service_delegate_;
  std::unique_ptr<PsstErrorReportUploader> report_uploader_;
};

}  //  namespace psst

#endif  // BRAVE_COMPONENTS_PSST_CORE_BROWSER_PSST_REPORTER_SERVICE_H_
