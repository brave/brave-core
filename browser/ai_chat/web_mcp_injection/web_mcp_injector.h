// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_AI_CHAT_WEB_MCP_INJECTION_WEB_MCP_INJECTOR_H_
#define BRAVE_BROWSER_AI_CHAT_WEB_MCP_INJECTION_WEB_MCP_INJECTOR_H_

#include <memory>
#include <string_view>

#include "brave/components/script_injector/common/mojom/script_injector.mojom.h"
#include "content/public/browser/web_contents_observer.h"
#include "mojo/public/cpp/bindings/associated_remote.h"

namespace content {
class Page;
class WebContents;
}  // namespace content

namespace ai_chat {

// Injects Brave-provided WebMCP tools (see GetWebMcpInjectionRules()) into
// pages whose URL matches a rule. The injected script calls
// `document.modelContext.registerTool(...)` in the page's main world; the
// existing ContentTool pipeline then discovers the tool automatically.
//
// One instance is scoped to a single tab's WebContents. It is owned by
// BraveTabFeatures (desktop and Android) and created via MaybeCreate().
//
// Proof-of-concept: rules are hardcoded and injected into the main world.
class WebMcpInjector : public content::WebContentsObserver {
 public:
  // Creates an injector for `web_contents`, or returns nullptr when WebMCP is
  // unavailable (the runtime feature is disabled, or there are no rules). The
  // injected script also guards against a missing document.modelContext, so
  // this is a cheap early-out.
  static std::unique_ptr<WebMcpInjector> MaybeCreate(
      content::WebContents* web_contents);

  explicit WebMcpInjector(content::WebContents* web_contents);
  ~WebMcpInjector() override;

  WebMcpInjector(const WebMcpInjector&) = delete;
  WebMcpInjector& operator=(const WebMcpInjector&) = delete;

 private:
  // content::WebContentsObserver:
  void DocumentOnLoadCompletedInPrimaryMainFrame() override;
  void PrimaryPageChanged(content::Page& page) override;

  void InjectScript(std::string_view script);

  mojo::AssociatedRemote<script_injector::mojom::ScriptInjector>
      script_injector_remote_;
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_AI_CHAT_WEB_MCP_INJECTION_WEB_MCP_INJECTOR_H_
