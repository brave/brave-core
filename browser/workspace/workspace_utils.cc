/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/workspace/workspace_utils.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/files/file_util.h"
#include "base/logging.h"

void WriteWorkspaceToDisk(
    std::vector<std::unique_ptr<sessions::SessionCommand>> commands,
    const base::FilePath& workspace_dir,
    scoped_refptr<sessions::CommandStorageBackend> backend,
    base::OnceClosure on_error) {
  if (!base::CreateDirectory(workspace_dir)) {
    DVLOG(1) << "Failed to create workspace directory: " << workspace_dir;
    std::move(on_error).Run();
    return;
  }
  // AppendCommands posts |on_error| to the backend's callback_task_runner_
  // (the UI thread, set when the backend was constructed) on write failure.
  backend->AppendCommands(std::move(commands), /*truncate=*/true,
                          std::move(on_error));
}

std::vector<std::unique_ptr<sessions::SessionCommand>> ReadWorkspaceFromDisk(
    const base::FilePath& workspace_dir,
    scoped_refptr<sessions::CommandStorageBackend> backend) {
  // Only do file I/O here.  Callers must call RestoreSessionFromCommands() on
  // the UI thread because SessionTab/SessionWindow constructors call
  // SessionID::NewUnique() which is sequence-checked to the UI thread.
  sessions::CommandStorageBackend::ReadCommandsResult result =
      backend->ReadLastSessionCommands();
  if (result.error_reading || result.commands.empty()) {
    DVLOG(1) << "Could not read workspace session from: " << workspace_dir;
    return {};
  }
  return std::move(result.commands);
}
