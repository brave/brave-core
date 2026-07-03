// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/psst/core/browser/psst_reporter_service.h"

#include "brave/components/psst/core/browser/psst_report_uploader.h"

namespace psst {

PsstReporterService::PsstReporterService(
    std::unique_ptr<PsstErrorReportUploader> report_uploader)
    : report_uploader_(std::move(report_uploader)) {}

PsstReporterService::~PsstReporterService() = default;

void PsstReporterService::SubmitPsstErrorsReport(
    std::optional<PolicyTasksSet> failed_policy_tasks) {
  LOG(INFO) << "[PSST] PsstReporterService::SubmitPsstErrorsReport "
               "failed_policy_tasks.size:"
            << (failed_policy_tasks.has_value() ? failed_policy_tasks->size()
                                                : -1);
}

bool PolicyTaskCompare::operator()(const PolicyTask& lhs,
                                   const PolicyTask& rhs) const {
  return lhs.uid < rhs.uid;
}

}  // namespace psst
