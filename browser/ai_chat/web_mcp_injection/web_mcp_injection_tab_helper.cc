// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/web_mcp_injection/web_mcp_injection_tab_helper.h"

#include <string>

#include "base/feature_list.h"
#include "base/functional/callback_helpers.h"
#include "base/json/json_writer.h"
#include "base/strings/pattern.h"
#include "base/strings/strcat.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "brave/browser/ai_chat/web_mcp_injection/web_mcp_injection_rule.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"
#include "third_party/blink/public/common/features.h"
#include "third_party/blink/public/mojom/script/script_evaluation_params.mojom.h"
#include "url/gurl.h"
#include "url/url_constants.h"

namespace ai_chat {

namespace {

// Main world. navigator.modelContext must be registered on the page's own
// world so the document's ModelContext supplement (read by the AI page content
// pipeline) sees the tool.
constexpr int32_t kMainWorldId = 0;

// Builds a self-contained script that registers `rule` as a WebMCP tool. The
// script is a no-op when the WebMCP runtime feature is not enabled on the page
// (navigator.modelContext is then undefined).
std::string BuildRegisterToolScript(const WebMcpInjectionRule& rule) {
  // base::WriteJson produces a safely-escaped JS/JSON string literal.
  const std::string name_literal =
      base::WriteJson(base::Value(rule.tool_name)).value_or("\"\"");
  const std::string description_literal =
      base::WriteJson(base::Value(rule.description)).value_or("\"\"");

  return base::StrCat({
      "(function () {",
      "  if (!navigator.modelContext || !navigator.modelContext.registerTool)",
      "    return;",
      "  try {",
      "    navigator.modelContext.registerTool({",
      "      name: ",
      name_literal,
      ",",
      "      description: ",
      description_literal,
      ",",
      "      inputSchema: ",
      rule.input_schema,
      ",",
      "      execute: async (input) => {",
      rule.execute_body,
      "},",
      "    });",
      "  } catch (e) { /* PoC: ignore registration errors */ }",
      "})();",
  });
}

}  // namespace

// static
void WebMcpInjectionTabHelper::MaybeCreateForWebContents(
    content::WebContents* web_contents) {
  // kWebMCP's base::Feature gates the runtime feature; if it is force-disabled
  // there is no point injecting. The script itself also guards against a
  // missing navigator.modelContext, so this is a cheap early-out.
  if (!base::FeatureList::IsEnabled(blink::features::kWebMCP) ||
      GetWebMcpInjectionRules().empty()) {
    return;
  }
  CreateForWebContents(web_contents);
}

WebMcpInjectionTabHelper::WebMcpInjectionTabHelper(
    content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      content::WebContentsUserData<WebMcpInjectionTabHelper>(*web_contents) {}

WebMcpInjectionTabHelper::~WebMcpInjectionTabHelper() = default;

void WebMcpInjectionTabHelper::DocumentOnLoadCompletedInPrimaryMainFrame() {
  const GURL& url = web_contents()->GetLastCommittedURL();
  // Only inject into secure pages. WebMCP itself is restricted to secure
  // contexts, and tool scripts should never run over plaintext http.
  if (!url.SchemeIs(url::kHttpsScheme)) {
    return;
  }

  for (const auto& rule : GetWebMcpInjectionRules()) {
    // Glob match (base::MatchPattern '*'/'?' wildcards) against the full URL.
    if (base::MatchPattern(url.spec(), rule.url_pattern)) {
      InjectScript(BuildRegisterToolScript(rule));
    }
  }
}

void WebMcpInjectionTabHelper::PrimaryPageChanged(content::Page& page) {
  // Rebind the remote for the new page.
  script_injector_remote_.reset();
}

void WebMcpInjectionTabHelper::InjectScript(const std::string& script) {
  content::RenderFrameHost* rfh = web_contents()->GetPrimaryMainFrame();
  if (!rfh || !rfh->IsRenderFrameLive()) {
    return;
  }

  if (!script_injector_remote_.is_bound() ||
      !script_injector_remote_.is_connected()) {
    script_injector_remote_.reset();
    rfh->GetRemoteAssociatedInterfaces()->GetInterface(
        &script_injector_remote_);
    script_injector_remote_.reset_on_disconnect();
  }

  script_injector_remote_->RequestAsyncExecuteScript(
      kMainWorldId, base::UTF8ToUTF16(script),
      blink::mojom::UserActivationOption::kDoNotActivate,
      blink::mojom::PromiseResultOption::kDoNotWait, base::DoNothing());
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(WebMcpInjectionTabHelper);

}  // namespace ai_chat
