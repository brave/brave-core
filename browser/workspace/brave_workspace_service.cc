/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/workspace/brave_workspace_service.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include "base/files/file_enumerator.h"
#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "base/values.h"
#include "chrome/browser/profiles/profile.h"
#include "components/sessions/core/session_service_commands.h"

namespace {

// Name of the tiny metadata file stored inside each workspace directory.
constexpr char kInfoFileName[] = "info.json";
constexpr char kInfoKeyName[] = "name";

}  // namespace

BraveWorkspaceService::BraveWorkspaceService(Profile* profile)
    : profile_(profile) {}

BraveWorkspaceService::~BraveWorkspaceService() = default;

std::vector<WorkspaceInfo> BraveWorkspaceService::ListWorkspaces() const {
  return ListWorkspacesInDir(WorkspacesDir());
}

bool BraveWorkspaceService::DeleteWorkspace(const std::string& name) {
  base::FilePath dir = WorkspaceDirForName(name);
  if (!base::PathExists(dir)) {
    return true;
  }
  return base::DeletePathRecursively(dir);
}

base::FilePath BraveWorkspaceService::GetWorkspacesDir() const {
  return WorkspacesDir();
}

base::FilePath BraveWorkspaceService::GetWorkspaceDirForName(
    const std::string& name) const {
  return WorkspaceDirForName(name);
}

// static
bool BraveWorkspaceService::WriteWorkspaceToDisk(
    const std::string& name,
    std::vector<std::unique_ptr<sessions::SessionCommand>> commands,
    const base::FilePath& workspace_dir,
    scoped_refptr<sessions::CommandStorageBackend> backend) {
  if (name.empty() || commands.empty()) {
    return false;
  }

  if (!base::CreateDirectory(workspace_dir)) {
    LOG(ERROR) << "Failed to create workspace directory: " << workspace_dir;
    return false;
  }

  // Write the human-readable name to info.json so ListWorkspacesInDir can
  // recover the display name without parsing the binary session file.
  base::DictValue info;
  info.Set(kInfoKeyName, name);
  std::string info_json;
  if (!base::JSONWriter::Write(base::Value(std::move(info)), &info_json) ||
      !base::WriteFile(workspace_dir.AppendASCII(kInfoFileName), info_json)) {
    LOG(ERROR) << "Failed to write workspace info.json";
    return false;
  }

  // AppendCommands() is synchronous: it does file I/O directly on the calling
  // thread.  Pass a null error callback so the backend never attempts to
  // PostTask back to callback_task_runner_.
  backend->AppendCommands(std::move(commands), /*truncate=*/true,
                          base::OnceClosure());

  // IsFileOpenForTesting() is the only public success indicator: the file
  // stays open on success and is closed (open_file_ = null) on any I/O error.
  bool success = backend->IsFileOpenForTesting();
  if (!success) {
    LOG(ERROR) << "Failed to write workspace session file in: "
               << workspace_dir;
  }
  return success;
}

// static
std::vector<std::unique_ptr<sessions::SessionCommand>>
BraveWorkspaceService::ReadWorkspaceFromDisk(
    const base::FilePath& workspace_dir,
    scoped_refptr<sessions::CommandStorageBackend> backend) {
  // Only do file I/O here.  Callers must call RestoreSessionFromCommands() on
  // the UI thread because SessionTab/SessionWindow constructors call
  // SessionID::NewUnique() which is sequence-checked to the UI thread.
  sessions::CommandStorageBackend::ReadCommandsResult result =
      backend->ReadLastSessionCommands();
  if (result.error_reading || result.commands.empty()) {
    LOG(ERROR) << "Could not read workspace session from: " << workspace_dir;
    return {};
  }
  return std::move(result.commands);
}

// static
std::vector<WorkspaceInfo> BraveWorkspaceService::ListWorkspacesInDir(
    const base::FilePath& workspaces_dir) {
  std::vector<WorkspaceInfo> result;

  if (!base::DirectoryExists(workspaces_dir)) {
    return result;
  }

  // Enumerate direct subdirectories — each is a workspace.
  base::FileEnumerator dir_enum(workspaces_dir, /*recursive=*/false,
                                base::FileEnumerator::DIRECTORIES);
  for (base::FilePath workspace_dir = dir_enum.Next(); !workspace_dir.empty();
       workspace_dir = dir_enum.Next()) {
    // Read the display name from info.json.
    base::FilePath info_path = workspace_dir.AppendASCII(kInfoFileName);
    std::string info_json;
    if (!base::ReadFileToString(info_path, &info_json)) {
      continue;
    }
    auto parsed = base::JSONReader::ReadDict(
        info_json, base::JSON_PARSE_CHROMIUM_EXTENSIONS);
    if (!parsed) {
      continue;
    }
    const std::string* name = parsed->FindString(kInfoKeyName);
    if (!name || name->empty()) {
      continue;
    }

    // Derive created_at from the timestamp embedded in the Session_* filename.
    base::Time created_at;
    auto session_paths = sessions::CommandStorageBackend::GetSessionFilePaths(
        workspace_dir, kWorkspaceSessionType);
    for (const auto& session_path : session_paths) {
      base::Time t;
      if (sessions::CommandStorageBackend::TimestampFromPath(session_path, t) &&
          t > created_at) {
        created_at = t;
      }
    }
    if (created_at.is_null()) {
      continue;
    }

    WorkspaceInfo info;
    info.name = *name;
    info.created_at = created_at;
    result.push_back(std::move(info));
  }

  // Most-recently created first.
  std::sort(result.begin(), result.end(),
            [](const WorkspaceInfo& a, const WorkspaceInfo& b) {
              return a.created_at > b.created_at;
            });
  return result;
}

// static
std::string BraveWorkspaceService::SanitizeName(const std::string& name) {
  std::string sanitized;
  sanitized.reserve(name.size());
  for (char c : name) {
    if (base::IsAsciiAlphaNumeric(c) || c == '-' || c == '_') {
      sanitized += base::ToLowerASCII(c);
    } else if (c == ' ') {
      sanitized += '-';
    }
    if (sanitized.size() >= 64) {
      break;
    }
  }
  if (sanitized.empty()) {
    sanitized = "workspace";
  }
  return sanitized;
}

base::FilePath BraveWorkspaceService::WorkspacesDir() const {
  return profile_->GetPath().AppendASCII("workspaces");
}

base::FilePath BraveWorkspaceService::WorkspaceDirForName(
    const std::string& name) const {
  return WorkspacesDir().AppendASCII(SanitizeName(name));
}
