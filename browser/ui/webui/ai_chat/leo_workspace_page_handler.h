// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_AI_CHAT_LEO_WORKSPACE_PAGE_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_AI_CHAT_LEO_WORKSPACE_PAGE_HANDLER_H_

#include <string>
#include <string_view>

#include "base/memory/raw_ptr.h"
#include "brave/components/ai_chat/core/common/mojom/leo_workspace.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"

namespace content {
class RenderFrameHost;
class WebUI;
}  // namespace content

namespace ai_chat {

// Finds the frame displaying a workspace file for |workspace_frame|, the
// chrome-untrusted://<uuid>.leo-workspace document. The workspace displays a
// file by framing its viewer document (on chrome-untrusted://view.<uuid>.
// leo-workspace), which in turn frames the file itself at
// chrome-untrusted://view.<uuid>.leo-workspace/<path>, served from the
// workspace folder by the viewer's service worker:
//
//   workspace frame  <uuid>.leo-workspace/
//   └ viewer frame   view.<uuid>.leo-workspace/#file=<path>, named
//     |                |viewer_frame_name|
//     └ file frame   view.<uuid>.leo-workspace/<path>
//
// Only a viewer frame that is a direct child of |workspace_frame|, and a file
// frame that is a direct child of that, both on the workspace's own `view.`
// origin, are matched. Returns nullptr if there is no such file frame.
content::RenderFrameHost* FindLeoWorkspaceFileFrame(
    content::RenderFrameHost* workspace_frame,
    std::string_view viewer_frame_name);

// Browser side of the chrome-untrusted://<uuid>.leo-workspace page. Bound only
// by LeoWorkspaceUI, so never by the workspace's `view.` origin, which renders
// arbitrary workspace content.
class LeoWorkspacePageHandler : public mojom::LeoWorkspaceHandler {
 public:
  LeoWorkspacePageHandler(
      content::WebUI* web_ui,
      mojo::PendingReceiver<mojom::LeoWorkspaceHandler> receiver);
  ~LeoWorkspacePageHandler() override;

  LeoWorkspacePageHandler(const LeoWorkspacePageHandler&) = delete;
  LeoWorkspacePageHandler& operator=(const LeoWorkspacePageHandler&) = delete;

  // mojom::LeoWorkspaceHandler:
  void PrintFileToPdf(const std::string& viewer_frame_name,
                      bool landscape,
                      PrintFileToPdfCallback callback) override;

 private:
  const raw_ptr<content::WebUI> web_ui_;
  mojo::Receiver<mojom::LeoWorkspaceHandler> receiver_;
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_UI_WEBUI_AI_CHAT_LEO_WORKSPACE_PAGE_HANDLER_H_
