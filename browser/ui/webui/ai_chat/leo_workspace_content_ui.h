// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_AI_CHAT_LEO_WORKSPACE_CONTENT_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_AI_CHAT_LEO_WORKSPACE_CONTENT_UI_H_

#include <memory>
#include <string>

#include "base/files/file_path.h"
#include "content/public/browser/webui_config.h"
#include "ui/webui/untrusted_web_ui_controller.h"

namespace ai_chat {

class LeoWorkspaceContentUIConfig : public content::WebUIConfig {
 public:
  LeoWorkspaceContentUIConfig();
  ~LeoWorkspaceContentUIConfig() override;

  // content::WebUIConfig:
  bool IsWebUIEnabled(content::BrowserContext* browser_context) override;
  std::unique_ptr<content::WebUIController> CreateWebUIController(
      content::WebUI* web_ui,
      const GURL& url) override;
};

// Serves the contents of a workspace folder at
// chrome-untrusted://leo-workspace-content/<workspace-uuid>/<relative path>, so
// that files the model wrote can be previewed in an iframe. The folder for a
// given uuid comes from WorkspaceContentRegistry, which is populated by
// WorkspaceAssociatedContent for as long as that workspace exists.
//
// Serving real URL paths (rather than blob: URLs) is what makes relative
// references between generated files resolve: an <img src="./logo.png"> in
// /<uuid>/index.html resolves to /<uuid>/logo.png and is served from the same
// folder.
//
// This is a different origin from chrome-untrusted://leo-workspace on purpose.
// That page is auto-granted File System Access read/write, and the content
// served here is model-authored, so the two must not share an origin.
class LeoWorkspaceContentUI : public ui::UntrustedWebUIController {
 public:
  explicit LeoWorkspaceContentUI(content::WebUI* web_ui);
  ~LeoWorkspaceContentUI() override;

  LeoWorkspaceContentUI(const LeoWorkspaceContentUI&) = delete;
  LeoWorkspaceContentUI& operator=(const LeoWorkspaceContentUI&) = delete;
};

// Splits a request path of the form "<uuid>/<relative path>" into its parts.
// Any query or fragment is discarded, percent-escapes are decoded, and a
// request for a directory root is mapped to "index.html".
//
// On success |relative_path| is guaranteed to be relative and free of parent
// references. Note that this is not the same as rejecting traversal attempts:
// URL canonicalisation resolves dot segments (including percent-encoded ones)
// before this function sees the path, so "<uuid>/../secret" arrives already
// collapsed to "secret" and is parsed as a request for workspace "secret" -
// which matches no registered workspace, and so serves nothing.
//
// Returns false - leaving the out params untouched - if the path is empty or a
// segment survives canonicalisation that could still escape the folder (an
// escaped separator, a control character, or a literal dot segment).
//
// Exposed for testing.
bool ParseWorkspaceContentPath(const std::string& request_path,
                               std::string* uuid,
                               base::FilePath* relative_path);

// Reads |relative_path| under |folder|, returning empty contents if the file
// does not exist or resolves (after following symlinks) outside |folder|.
//
// Blocking; must be called on a thread that allows IO. Exposed for testing.
std::string ReadWorkspaceFileBlocking(const base::FilePath& folder,
                                      const base::FilePath& relative_path);

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_UI_WEBUI_AI_CHAT_LEO_WORKSPACE_CONTENT_UI_H_
