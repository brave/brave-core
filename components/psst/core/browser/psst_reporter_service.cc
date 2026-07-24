// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/psst/core/browser/psst_reporter_service.h"

#include <utility>

#include "base/task/sequenced_task_runner.h"
#include "base/values.h"
#include "brave/components/psst/core/browser/psst_report_uploader.h"
#include "brave/components/version_info/version_info.h"

namespace psst {

bool PolicyTaskCompare::operator()(const PolicyTask& lhs,
                                   const PolicyTask& rhs) const {
  return lhs.uid < rhs.uid;
}

PsstReporterService::PsstReporterService(
    ChannelNameCallback channel_name_callback,
    ComponentVersionCallback component_version_callback,
    std::unique_ptr<PsstErrorReportUploader> report_uploader)
    : channel_name_callback_(std::move(channel_name_callback)),
      component_version_callback_(std::move(component_version_callback)),
      report_uploader_(std::move(report_uploader)) {
  CHECK(channel_name_callback_);
  CHECK(component_version_callback_);
}

PsstReporterService::~PsstReporterService() = default;

void PsstReporterService::SubmitPsstErrorsReport(
    std::optional<PolicyTasksSet> failed_policy_tasks,
    const int script_version,
    OnSubmitPsstErrorsReportCallback callback) {
  if (!failed_policy_tasks || failed_policy_tasks->empty()) {
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, base::BindOnce(std::move(callback)));
    return;
  }

  auto brave_version =
      version_info::GetBraveVersionWithoutChromiumMajorVersion();

  std::optional<std::string> channel = channel_name_callback_.Run();

  std::optional<std::string> psst_component_version =
      component_version_callback_.Run();

  base::ListValue failed_tasks;
  for (const auto& ti : failed_policy_tasks.value()) {
    if (ti.error_description) {
      failed_tasks.Append(ti.ToValue());
    }
  }

  if (failed_tasks.empty()) {
    // Nothing to report
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, base::BindOnce(std::move(callback)));
    return;
  }

  report_uploader_->Upload(std::move(psst_component_version), script_version,
                           brave_version, std::move(channel),
                           std::move(failed_tasks), std::move(callback));
}

}  // namespace psst
