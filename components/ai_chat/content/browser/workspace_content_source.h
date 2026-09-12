// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_WORKSPACE_CONTENT_SOURCE_H_
#define BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_WORKSPACE_CONTENT_SOURCE_H_

#include <optional>
#include <string>
#include <string_view>

#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "content/public/browser/url_data_source.h"

namespace content {
class BrowserContext;
}

namespace ai_chat {

// Serves a workspace folder (from WorkspaceContentRegistry) at
// chrome-untrusted://leo-workspace/<uuid>/files/<path relative to the folder>.
//
// SECURITY: served files share the leo-workspace origin with the workspace tool
// pages, which are auto-granted File System Access read/write and are the only
// origin allowed navigator.modelContext, so model-authored script served from
// here runs with both. CSP cannot help: it is per-data-source, so served files
// and tool pages share one policy.
// TODO(https://github.com/brave/brave-browser/issues/58792): Support dynamic
// origins from WebUI, so each workspace's files get an origin of their own.
//
// Only user-picked folders are supported; OPFS-backed workspaces (needed for
// Android) will need a second backend via
// StoragePartition::GetFileSystemContext().

// Pass both to WebUIDataSource::SetRequestFilter(). |request_path| is a
// URLToRequestPath() value, so still percent-encoded and possibly carrying a
// query and ref.
//
// Malformed "<uuid>/files/..." requests are claimed too: a declined request
// falls through to the default resource, which would answer a bad file path
// with the tool page rather than an error.
bool ShouldHandleWorkspaceFileRequest(const std::string& request_path);

// Replies with null, which the WebUI loader turns into ERR_FAILED, for anything
// that does not resolve to a readable regular file inside a registered folder.
void HandleWorkspaceFileRequest(
    base::WeakPtr<content::BrowserContext> browser_context,
    const std::string& request_path,
    content::URLDataSource::GotDataCallback callback);

// Everything below is exposed for testing.

// Splits "<uuid>/files/<path>", returning std::nullopt unless the result names
// a file that cannot escape the folder. GURL has already collapsed dot segments
// by this point, so "/../secret" arrives as "/secret": a file that will not
// exist, not traversal.
struct WorkspaceFileRequest {
  std::string uuid;
  base::FilePath relative_path;
};
std::optional<WorkspaceFileRequest> ParseWorkspaceFileRequestPath(
    std::string_view request_path);

// Returns the absolute path to read, or std::nullopt if the target is not a
// readable regular file that still resolves inside |folder| after symlinks.
// Blocking; must run on a thread that allows IO.
std::optional<base::FilePath> ResolveWorkspaceFileBlocking(
    const base::FilePath& folder,
    const base::FilePath& relative_path);

// A data source hands back one buffer, so an oversized file would sit in
// browser memory entire rather than streaming. Refused instead.
inline constexpr int64_t kMaxWorkspaceFileSize = 64 * 1024 * 1024;

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_WORKSPACE_CONTENT_SOURCE_H_
