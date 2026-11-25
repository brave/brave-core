// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_AI_CHAT_LEO_WORKSPACE_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_AI_CHAT_LEO_WORKSPACE_UI_H_

#include <memory>
#include <string_view>

#include "brave/components/ai_chat/core/common/mojom/leo_workspace.mojom-forward.h"
#include "content/public/browser/webui_config.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "ui/webui/untrusted_web_ui_controller.h"
#include "url/origin.h"

namespace ai_chat {

class LeoWorkspacePageHandler;

// The origin of the viewer document of the workspace served from
// |workspace_host| (<uuid>.leo-workspace):
// chrome-untrusted://view.<uuid>.leo-workspace.
url::Origin GetLeoWorkspaceViewerOrigin(std::string_view workspace_host);

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
  std::unique_ptr<content::WebUIController> CreateWebUIController(
      content::WebUI* web_ui,
      const GURL& url) override;
  void RegisterURLDataSource(content::BrowserContext* browser_context) override;
  // The viewer's files are served by a service worker, so navigations to this
  // host must reach it rather than taking the WebUI shortcut.
  bool ShouldInterceptNavigationsWithServiceWorker() override;
};

// Hidden, headless Untrusted WebUI that hosts the Leo "workspace" tools. The
// page receives a FileSystemDirectoryHandle (delivered by the browser via
// launchQueue) for a user-picked folder, implements the file tools in
// JavaScript against it, and registers them with Leo via WebMCP
// (document.modelContext). One instance is created per conversation and served
// from its own origin at chrome-untrusted://<guid>.leo-workspace; it runs with
// a locked-down CSP that permits only its own first-party bundle and framing
// its own viewer document. This page has no visible UI of its own. For what
// the page can't do itself, such as printing a file to PDF, it binds
// mojom::LeoWorkspaceHandler. Its viewer document cannot.
class LeoWorkspaceUI : public ui::UntrustedWebUIController {
 public:
  LeoWorkspaceUI(content::WebUI* web_ui, const GURL& url);
  ~LeoWorkspaceUI() override;

  LeoWorkspaceUI(const LeoWorkspaceUI&) = delete;
  LeoWorkspaceUI& operator=(const LeoWorkspaceUI&) = delete;

  void BindInterface(
      mojo::PendingReceiver<mojom::LeoWorkspaceHandler> receiver);

 private:
  std::unique_ptr<LeoWorkspacePageHandler> page_handler_;

  WEB_UI_CONTROLLER_TYPE_DECL();
};

// The document a workspace is allowed to frame to display its contents, served
// from chrome-untrusted://view.<guid>.leo-workspace. Only its workspace frames
// it, whether hidden or shown in AI Chat's lightbox or conversation frame.
// Being a different origin from the workspace it belongs to, it has none of
// that workspace's File System Access grants, storage or WebMCP, and no other
// page may frame it. Anything it needs has to cross the frame boundary.
class LeoWorkspaceViewUI : public ui::UntrustedWebUIController {
 public:
  LeoWorkspaceViewUI(content::WebUI* web_ui, const GURL& url);
  ~LeoWorkspaceViewUI() override;

  LeoWorkspaceViewUI(const LeoWorkspaceViewUI&) = delete;
  LeoWorkspaceViewUI& operator=(const LeoWorkspaceViewUI&) = delete;
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_UI_WEBUI_AI_CHAT_LEO_WORKSPACE_UI_H_
