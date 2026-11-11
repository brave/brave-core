// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_AI_CHAT_WEB_MCP_INJECTION_WEB_MCP_INJECTOR_H_
#define BRAVE_BROWSER_AI_CHAT_WEB_MCP_INJECTION_WEB_MCP_INJECTOR_H_

#include <memory>
#include <string_view>

#include "brave/components/script_injector/common/mojom/script_injector.mojom.h"
#include "chrome/browser/ui/tabs/contents_observing_tab_feature.h"
#include "mojo/public/cpp/bindings/associated_remote.h"

namespace content {
class Page;
class WebContents;
}  // namespace content

namespace tabs {
class TabInterface;
}  // namespace tabs

namespace ai_chat {

// Injects Component-provided WebMCP tools into pages whose URL matches a rule.
// The rules are delivered by the component updater and held in
// web_mcp::WebMcpRuleRegistry. The injected script calls
// `document.modelContext.registerTool(...)` in the page's main world; the
// existing ContentTool pipeline then discovers the tool automatically.
//
// One instance is scoped to a single tab, and follows the tab's contents when
// they are replaced. This instance is owned by BraveTabFeatures.
class WebMcpInjector : public tabs::ContentsObservingTabFeature {
 public:
  // Creates an injector for `tab`, or returns nullptr when the WebMCP runtime
  // feature is disabled. The injected script also guards against a missing
  // document.modelContext, so this is a cheap early-out.
  static std::unique_ptr<WebMcpInjector> MaybeCreate(tabs::TabInterface& tab);

  explicit WebMcpInjector(tabs::TabInterface& tab);
  ~WebMcpInjector() override;

  WebMcpInjector(const WebMcpInjector&) = delete;
  WebMcpInjector& operator=(const WebMcpInjector&) = delete;

 private:
  // tabs::ContentsObservingTabFeature:
  void OnDiscardContents(tabs::TabInterface* tab,
                         content::WebContents* old_contents,
                         content::WebContents* new_contents) override;

  // content::WebContentsObserver:
  void DocumentOnLoadCompletedInPrimaryMainFrame() override;
  void PrimaryPageChanged(content::Page& page) override;

  void InjectScript(std::string_view script);

  mojo::AssociatedRemote<script_injector::mojom::ScriptInjector>
      script_injector_remote_;
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_AI_CHAT_WEB_MCP_INJECTION_WEB_MCP_INJECTOR_H_
