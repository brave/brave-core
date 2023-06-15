/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_FEDERATED_ADAPTERS_FLOWER_HELPER_H_
#define BRAVE_COMPONENTS_BRAVE_FEDERATED_ADAPTERS_FLOWER_HELPER_H_

#include <string>

#include "brave/components/brave_federated/task/typing.h"
#include "brave/third_party/flower/src/proto/flwr/proto/task.pb.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_federated {

std::string BuildGetTasksPayload();

absl::optional<Task> ParseTask(const flower::TaskIns& task_instruction);
absl::optional<TaskList> ParseTaskListFromResponseBody(
    const std::string& response_body);

std::string BuildUploadTaskResultsPayload(const TaskResult& result);

}  // namespace brave_federated

#endif  // BRAVE_COMPONENTS_BRAVE_FEDERATED_ADAPTERS_FLOWER_HELPER_H_
