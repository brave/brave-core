// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/psst/core/browser/psst_reporter_service.h"

#include "base/values.h"
#include "brave/components/psst/core/browser/psst_component_installer.h"
#include "brave/components/psst/core/browser/psst_report_uploader.h"

namespace psst {

PsstReporterService::PsstReporterService(
    std::unique_ptr<Delegate> service_delegate,
    std::unique_ptr<PsstErrorReportUploader> report_uploader)
    : service_delegate_(std::move(service_delegate)),
      report_uploader_(std::move(report_uploader)) {}

PsstReporterService::~PsstReporterService() = default;

void PsstReporterService::SubmitPsstErrorsReport(
    std::optional<PolicyTasksSet> failed_policy_tasks,
    const int script_version,
    OnSubmitPsstErrorsReportCallback callback) {
  if (!failed_policy_tasks || failed_policy_tasks->empty()) {
    std::move(callback).Run();
    return;
  }

  std::optional<std::string> psst_component_version;
  if (service_delegate_) {
    auto all_components = service_delegate_->GetComponentInfos();
    auto it =
        std::find_if(all_components.begin(), all_components.end(),
                     [](const auto& c) { return c.id == kPsstComponentId; });
    psst_component_version = (it != all_components.end())
                                 ? std::optional<std::string>(it->version)
                                 : std::nullopt;
  }

  base::ListValue failed_tasks;
  for (const auto& ti : failed_policy_tasks.value()) {
    if (ti.error_description) {
      failed_tasks.Append(ti.ToValue());
    }
  }

  if(failed_tasks.empty()) {
    // Nothing to report
    std::move(callback).Run();
    return;
  }

  LOG(INFO) << "[PSST] PsstReporterService::SubmitPsstErrorsReport "
               "\npsst_component_version:"
            << (psst_component_version ? psst_component_version.value() : "n/a")
            << "\nscript_version:" << script_version
            << "\nfailed_policy_tasks:" << failed_tasks.DebugString();

  report_uploader_->Upload(std::move(psst_component_version), script_version,
                           std::move(failed_tasks), std::move(callback));
}

bool PolicyTaskCompare::operator()(const PolicyTask& lhs,
                                   const PolicyTask& rhs) const {
  return lhs.uid < rhs.uid;
}

}  // namespace psst
