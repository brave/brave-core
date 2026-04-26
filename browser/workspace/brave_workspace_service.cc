/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/workspace/brave_workspace_service.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "base/values.h"
#include "chrome/browser/profiles/profile.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/sessions/core/session_service_commands.h"

namespace {

// Profile preference key — stores a dict keyed by sanitized workspace name.
const char kWorkspacesMetadataPref[] = "brave.workspaces";

// Keys within each per-workspace dict entry.
const char kWorkspaceName[] = "name";
const char kWorkspaceWindowCount[] = "number-of-windows";
const char kWorkspaceTabCount[] = "number-of-tabs";
const char kWorkspaceModifiedAt[] = "modifed-at";

}  // namespace

// static
void BraveWorkspaceService::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterDictionaryPref(kWorkspacesMetadataPref);
}

BraveWorkspaceService::BraveWorkspaceService(Profile* profile)
    : profile_(profile) {}

BraveWorkspaceService::~BraveWorkspaceService() = default;

std::vector<WorkspaceInfo> BraveWorkspaceService::ListWorkspaces() const {
  std::vector<WorkspaceInfo> result;
  const base::DictValue& all =
      profile_->GetPrefs()->GetDict(kWorkspacesMetadataPref);

  for (const auto [key, value] : all) {
    const base::DictValue* entry = value.GetIfDict();
    if (!entry) {
      continue;
    }
    const std::string* name = entry->FindString(kWorkspaceName);
    if (!name || name->empty()) {
      continue;
    }
    std::optional<double> modified_at = entry->FindDouble(kWorkspaceModifiedAt);
    if (!modified_at) {
      continue;
    }
    WorkspaceInfo info;
    info.name = *name;
    info.number_of_windows = entry->FindInt(kWorkspaceWindowCount).value_or(1);
    info.number_of_tabs = entry->FindInt(kWorkspaceTabCount).value_or(0);
    info.modified_at = base::Time::FromSecondsSinceUnixEpoch(*modified_at);
    result.push_back(std::move(info));
  }

  std::sort(result.begin(), result.end(),
            [](const WorkspaceInfo& a, const WorkspaceInfo& b) {
              return a.modified_at > b.modified_at;
            });
  return result;
}

void BraveWorkspaceService::SaveWorkspaceMetadata(const std::string& name,
                                                  int window_count,
                                                  int tab_count,
                                                  base::Time modified_at) {
  PrefService* prefs = profile_->GetPrefs();
  base::DictValue updated = prefs->GetDict(kWorkspacesMetadataPref).Clone();

  base::DictValue entry;
  entry.Set(kWorkspaceName, name);
  entry.Set(kWorkspaceWindowCount, window_count);
  entry.Set(kWorkspaceTabCount, tab_count);
  entry.Set(kWorkspaceModifiedAt, modified_at.InSecondsFSinceUnixEpoch());

  updated.Set(SanitizeName(name), std::move(entry));
  prefs->SetDict(kWorkspacesMetadataPref, std::move(updated));
}

void BraveWorkspaceService::RemoveWorkspaceMetadata(const std::string& name) {
  PrefService* prefs = profile_->GetPrefs();
  base::DictValue updated = prefs->GetDict(kWorkspacesMetadataPref).Clone();
  updated.Remove(SanitizeName(name));
  prefs->SetDict(kWorkspacesMetadataPref, std::move(updated));
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
    std::vector<std::unique_ptr<sessions::SessionCommand>> commands,
    const base::FilePath& workspace_dir,
    scoped_refptr<sessions::CommandStorageBackend> backend) {
  if (commands.empty()) {
    return false;
  }

  if (!base::CreateDirectory(workspace_dir)) {
    LOG(ERROR) << "Failed to create workspace directory: " << workspace_dir;
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
