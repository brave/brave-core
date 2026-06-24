// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_AI_CHAT_WEB_MCP_INJECTION_WEB_MCP_INJECTION_TAB_HELPER_H_
#define BRAVE_BROWSER_AI_CHAT_WEB_MCP_INJECTION_WEB_MCP_INJECTION_TAB_HELPER_H_

#include "brave/components/script_injector/common/mojom/script_injector.mojom.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "mojo/public/cpp/bindings/associated_remote.h"

namespace content {
class WebContents;
}  // namespace content

namespace ai_chat {

// Injects Brave-provided WebMCP tools (see GetWebMcpInjectionRules()) into
// pages whose URL matches a rule. The injected script calls
// `navigator.modelContext.registerTool(...)` in the page's main world; the
// existing ContentTool pipeline then discovers the tool automatically.
//
// Proof-of-concept: rules are hardcoded and injected into the main world.
class WebMcpInjectionTabHelper
    : public content::WebContentsObserver,
      public content::WebContentsUserData<WebMcpInjectionTabHelper> {
 public:
  WebMcpInjectionTabHelper(const WebMcpInjectionTabHelper&) = delete;
  WebMcpInjectionTabHelper& operator=(const WebMcpInjectionTabHelper&) = delete;
  ~WebMcpInjectionTabHelper() override;

  // Creates the helper for `web_contents` unless WebMCP is unavailable (e.g.
  // the runtime feature is disabled or there are no rules).
  static void MaybeCreateForWebContents(content::WebContents* web_contents);

 private:
  friend class content::WebContentsUserData<WebMcpInjectionTabHelper>;

  explicit WebMcpInjectionTabHelper(content::WebContents* web_contents);

  // content::WebContentsObserver:
  void DocumentOnLoadCompletedInPrimaryMainFrame() override;
  void PrimaryPageChanged(content::Page& page) override;

  void InjectScript(const std::string& script);

  mojo::AssociatedRemote<script_injector::mojom::ScriptInjector>
      script_injector_remote_;

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_AI_CHAT_WEB_MCP_INJECTION_WEB_MCP_INJECTION_TAB_HELPER_H_
