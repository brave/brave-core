// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_AI_CHAT_LEO_WORKSPACE_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_AI_CHAT_LEO_WORKSPACE_UI_H_

#include <memory>

#include "content/public/browser/webui_config.h"
#include "ui/webui/untrusted_web_ui_controller.h"

namespace ai_chat {

class LeoWorkspaceUIConfig : public content::WebUIConfig {
 public:
  LeoWorkspaceUIConfig();
  ~LeoWorkspaceUIConfig() override;

  // content::WebUIConfig:
  bool IsWebUIEnabled(content::BrowserContext* browser_context) override;
  // Every workspace is served from its own subdomain of the host this config is
  // registered for, so that each one is a separate origin. The workspace's
  // viewer document gets a subdomain of that in turn.
  bool ShouldHandleSubdomains() const override;
  // Serves only <uuid>.leo-workspace and view.<uuid>.leo-workspace, since
  // opting into subdomains means being handed every unclaimed host under this
  // one.
  bool ShouldHandleURL(const GURL& url) override;
  // Lets the viewer origins' service worker control viewer documents.
  bool ShouldInterceptNavigationsWithServiceWorker() override;
  std::unique_ptr<content::WebUIController> CreateWebUIController(
      content::WebUI* web_ui,
      const GURL& url) override;
};

// Hidden, headless Untrusted WebUI that hosts the Leo "workspace" tools. The
// page receives a FileSystemDirectoryHandle (delivered by the browser via
// launchQueue) for a user-picked folder, implements the file tools in
// JavaScript against it, and registers them with Leo via WebMCP
// (navigator.modelContext). One instance is created per conversation and served
// from its own origin at chrome-untrusted://<guid>.leo-workspace; it runs with
// a locked-down CSP that permits only its own first-party bundle and framing
// its own viewer document. This page has no visible UI of its own.
class LeoWorkspaceUI : public ui::UntrustedWebUIController {
 public:
  LeoWorkspaceUI(content::WebUI* web_ui, const GURL& url);
  ~LeoWorkspaceUI() override;

  LeoWorkspaceUI(const LeoWorkspaceUI&) = delete;
  LeoWorkspaceUI& operator=(const LeoWorkspaceUI&) = delete;
};

// The document a workspace is allowed to frame to display its contents, served
// from chrome-untrusted://view.<guid>.leo-workspace. Nothing frames it yet.
// Being a different origin from the workspace it belongs to, it has none of
// that workspace's File System Access grants, storage or WebMCP, and no other
// page may frame it. Anything it needs has to cross the frame boundary: its
// service worker turns /files/<path> URLs into READ_FILE postMessage requests
// to the parent workspace frame, which this controller registers for the
// origin browser-side, because a chrome-untrusted origin cannot register a
// worker from JavaScript.
class LeoWorkspaceViewUI : public ui::UntrustedWebUIController {
 public:
  LeoWorkspaceViewUI(content::WebUI* web_ui, const GURL& url);
  ~LeoWorkspaceViewUI() override;

  LeoWorkspaceViewUI(const LeoWorkspaceViewUI&) = delete;
  LeoWorkspaceViewUI& operator=(const LeoWorkspaceViewUI&) = delete;

 private:
  void RegisterServiceWorker(content::WebUI* web_ui, const GURL& url);
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_UI_WEBUI_AI_CHAT_LEO_WORKSPACE_UI_H_
