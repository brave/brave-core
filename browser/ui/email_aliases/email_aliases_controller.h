/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_EMAIL_ALIASES_EMAIL_ALIASES_CONTROLLER_H_
#define BRAVE_BROWSER_UI_EMAIL_ALIASES_EMAIL_ALIASES_CONTROLLER_H_

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/email_aliases/email_aliases_service.h"
#include "content/public/browser/global_routing_id.h"

class BrowserView;
class WebUIBubbleManager;

namespace content {
struct ContextMenuParams;
class RenderFrameHost;
}  // namespace content

namespace email_aliases {

class EmailAliasesController {
 public:
  EmailAliasesController(BrowserView* browser_view,
                         EmailAliasesService* email_aliases_service);
  ~EmailAliasesController();
  EmailAliasesController(const EmailAliasesController&) = delete;
  EmailAliasesController& operator=(const EmailAliasesController&) = delete;

  bool IsAvailableFor(const content::ContextMenuParams& params) const;

  void ShowBubble(content::RenderFrameHost* render_frame,
                  uint64_t field_renderer_id);
  void CloseBubble();
  void OpenSettingsPage();

  void OnAliasCreationComplete(const std::string& email);

  WebUIBubbleManager* GetBubbleForTesting();
  static void DisableAutoCloseBubbleForTesting(bool disale_autoclose);

 private:
  raw_ptr<BrowserView> browser_view_ = nullptr;
  raw_ptr<EmailAliasesService> email_aliases_service_ = nullptr;

  content::GlobalRenderFrameHostId field_render_frame_host_id_;
  uint64_t field_renderer_id_ = 0;

  std::unique_ptr<WebUIBubbleManager> bubble_;
  base::WeakPtrFactory<EmailAliasesController> weak_factory_{this};
};

}  // namespace email_aliases

#endif  // BRAVE_BROWSER_UI_EMAIL_ALIASES_EMAIL_ALIASES_CONTROLLER_H_
