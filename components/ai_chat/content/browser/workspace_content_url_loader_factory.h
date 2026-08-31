// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_WORKSPACE_CONTENT_URL_LOADER_FACTORY_H_
#define BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_WORKSPACE_CONTENT_URL_LOADER_FACTORY_H_

#include <optional>
#include <string>

#include "base/files/file_path.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "services/network/public/mojom/url_loader_factory.mojom-forward.h"
#include "url/gurl.h"

namespace content {
class BrowserContext;
}

namespace ai_chat {

// Serves a workspace folder at brave-leo-workspace://<workspace-uuid>/<path>,
// so files the model wrote can be previewed in an iframe. Folders come from
// WorkspaceContentRegistry, populated by WorkspaceAssociatedContent.
//
// Serving real URL paths rather than blob: URLs is what makes relative
// references between generated files resolve.
//
// Each uuid is a separate host and therefore a separate origin: that is what
// keeps model-authored content off chrome-untrusted://leo-workspace, which is
// auto-granted File System Access read/write.
//
// |restrict_to_uuid|, when set, is the only workspace this factory will serve.
// Pass it whenever the consumer is itself a workspace frame, so that one
// preview cannot read another's files by uuid; the commit-scheme grant is
// scheme-wide, so nothing below this factory would otherwise stop it. Leave it
// empty only for navigations, where the target workspace is by definition not
// the frame's current origin.
//
// Requests are rewritten to file:// and delegated to
// CreateFileURLLoaderBypassingSecurityChecks(), which streams through a data
// pipe and handles Range. That call bypasses security checks, hence the
// confinement in ResolveWorkspaceFileBlocking().
mojo::PendingRemote<network::mojom::URLLoaderFactory>
CreateWorkspaceContentURLLoaderFactory(
    content::BrowserContext* browser_context,
    std::optional<std::string> restrict_to_uuid);

// Splits a workspace URL into its uuid (the host) and the path relative to that
// workspace's folder, mapping a directory root to "index.html".
//
// |relative_path| comes back free of parent references, but note that is not
// the same as rejecting traversal: the scheme is registered standard, so GURL
// canonicalises the path as it would for http(s) - backslashes become slashes
// and dot segments are collapsed - before this sees it. "/../secret" arrives as
// "/secret" and is just a request for a file that will not exist.
//
// Returns std::nullopt for a malformed URL, or a segment that could still
// escape the folder once decoded. Exposed for testing.
struct WorkspaceContentPath {
  std::string uuid;
  base::FilePath relative_path;
};
std::optional<WorkspaceContentPath> ParseWorkspaceContentURL(const GURL& url);

// Returns the absolute path to read, or std::nullopt when the target does not
// exist, is not a regular file, is not readable, or resolves (after following
// symlinks) outside |folder|.
//
// Blocking; must run on a thread that allows IO. Exposed for testing.
std::optional<base::FilePath> ResolveWorkspaceFileBlocking(
    const base::FilePath& folder,
    const base::FilePath& relative_path);

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_WORKSPACE_CONTENT_URL_LOADER_FACTORY_H_
