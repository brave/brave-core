/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/workspaces/workspace_utils.h"

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

#include "base/files/file_util.h"
#include "base/json/values_util.h"
#include "base/logging.h"
#include "base/time/time.h"

namespace {

constexpr const char kWorkspaceName[] = "name";
constexpr const char kWorkspaceWindowCount[] = "number-of-windows";
constexpr const char kWorkspaceTabCount[] = "number-of-tabs";
constexpr const char kWorkspaceModifiedAt[] = "modified-at";

}  // namespace

std::vector<WorkspaceMetadata> ListWorkspacesFromDict(
    const base::DictValue& dict) {
  std::vector<WorkspaceMetadata> result;
  for (const auto [key, value] : dict) {
    const base::DictValue* entry = value.GetIfDict();
    if (!entry) {
      continue;
    }
    const std::string* name = entry->FindString(kWorkspaceName);
    if (!name || name->empty()) {
      continue;
    }
    WorkspaceMetadata info = {
        .name = *name,
        .modified_at = base::ValueToTime(entry->Find(kWorkspaceModifiedAt))
                           .value_or(base::Time::Now()),
        .number_of_windows = entry->FindInt(kWorkspaceWindowCount).value_or(1),
        .number_of_tabs = entry->FindInt(kWorkspaceTabCount).value_or(0)};
    result.push_back(std::move(info));
  }

  std::sort(result.begin(), result.end(),
            [](const WorkspaceMetadata& a, const WorkspaceMetadata& b) {
              return a.modified_at > b.modified_at;
            });
  return result;
}

base::DictValue WorkspaceMetadataToDictEntry(const WorkspaceMetadata& meta) {
  return base::DictValue()
      .Set(kWorkspaceName, meta.name)
      .Set(kWorkspaceWindowCount, meta.number_of_windows)
      .Set(kWorkspaceTabCount, meta.number_of_tabs)
      .Set(kWorkspaceModifiedAt, base::TimeToValue(meta.modified_at));
}

void WriteWorkspaceToDisk(
    std::vector<std::unique_ptr<sessions::SessionCommand>> commands,
    const base::FilePath& workspace_path,
    scoped_refptr<sessions::CommandStorageBackend> backend,
    base::OnceClosure on_error) {
  if (!base::CreateDirectory(workspace_path)) {
    DVLOG(1) << "Failed to create workspace directory: " << workspace_path;
    std::move(on_error).Run();
    return;
  }
  // AppendCommands posts |on_error| to the backend's callback_task_runner_
  // (the UI thread, set when the backend was constructed) on write failure.
  backend->AppendCommands(std::move(commands), /*truncate=*/true,
                          std::move(on_error));
}

std::vector<std::unique_ptr<sessions::SessionCommand>> ReadWorkspaceFromDisk(
    const base::FilePath& workspace_path,
    scoped_refptr<sessions::CommandStorageBackend> backend) {
  // Only do file I/O here.  Callers must call RestoreSessionFromCommands() on
  // the UI thread because SessionTab/SessionWindow constructors call
  // SessionID::NewUnique() which is sequence-checked to the UI thread.
  sessions::CommandStorageBackend::ReadCommandsResult result =
      backend->ReadLastSessionCommands();
  if (result.error_reading || result.commands.empty()) {
    DVLOG(1) << "Could not read workspace session from: " << workspace_path;
    return {};
  }
  return std::move(result.commands);
}
