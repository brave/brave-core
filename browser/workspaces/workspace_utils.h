/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_WORKSPACES_WORKSPACE_UTILS_H_
#define BRAVE_BROWSER_WORKSPACES_WORKSPACE_UTILS_H_

#include <memory>
#include <vector>

#include "base/files/file_path.h"
#include "base/functional/callback.h"
#include "base/memory/scoped_refptr.h"
#include "base/values.h"
#include "brave/browser/workspaces/workspace_metadata.h"
#include "components/sessions/core/command_storage_backend.h"
#include "components/sessions/core/command_storage_manager.h"
#include "components/sessions/core/session_command.h"

// Session type used for all workspace files.  Exposed so callers can
// construct a CommandStorageBackend on the UI thread before posting I/O
// work to a background sequenced runner.
inline constexpr sessions::CommandStorageManager::SessionType
    kWorkspaceSessionType =
        sessions::CommandStorageManager::SessionType::kSessionRestore;

// Writes |commands| to a Chromium session-command binary file via |backend|.
// |on_error| is called directly if the workspace directory cannot be created,
// or is posted to the UI thread (via AppendCommands' callback_task_runner_) on
// write failure.  Wrap with BindPostTask in the caller so it always lands on
// the UI thread regardless of which path fires.
//
// Must run on a background sequenced task runner.
void WriteWorkspaceToDisk(
    std::vector<std::unique_ptr<sessions::SessionCommand>> commands,
    const base::FilePath& workspace_path,
    scoped_refptr<sessions::CommandStorageBackend> backend,
    base::OnceClosure on_error);

// Reads the session-command binary via |backend| and returns the raw commands.
// Only does file I/O — callers must deserialize the commands into
// SessionWindow objects on the UI thread (SessionID::NewUnique() is
// sequence-checked to the UI thread).  Returns an empty vector on error.
//
// Must run on a background sequenced task runner.
std::vector<std::unique_ptr<sessions::SessionCommand>> ReadWorkspaceFromDisk(
    const base::FilePath& workspace_path,
    scoped_refptr<sessions::CommandStorageBackend> backend);

// Parses a workspace metadata dict (the value stored under
// kWorkspacesMetadataPref) and returns a list sorted by modified_at descending.
std::vector<WorkspaceMetadata> ListWorkspacesFromDict(
    const base::DictValue& dict);

// Serializes |meta| into a single dict entry suitable for storing under its key
// in kWorkspacesMetadataPref.
base::DictValue WorkspaceMetadataToDictEntry(const WorkspaceMetadata& meta);

#endif  // BRAVE_BROWSER_WORKSPACES_WORKSPACE_UTILS_H_
