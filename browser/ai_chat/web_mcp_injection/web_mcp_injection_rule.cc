// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/web_mcp_injection/web_mcp_injection_rule.h"

#include "base/containers/span.h"

namespace ai_chat {

namespace {

// PoC rule table. Each entry injects one WebMCP tool into pages matching
// `url_pattern`. The `execute_body` runs in the page's main world.
constexpr WebMcpInjectionRule kRules[] = {
    // Gmail: report the inbox unread count shown in bold next to the "Inbox"
    // label in the left navigation. Reads the DOM rather than any Gmail API.
    {
        .url_pattern = "https://mail.google.com/*",
        .tool_name = "unread_count",
        .description =
            "Report the number of unread messages in the Gmail inbox, read "
            "from "
            "the bold count next to the Inbox label in the left navigation.",
        // Tools that take no input still need a JSON Schema object with an
        // (empty) `properties` key. Without `properties`, ContentTool leaves
        // its input schema unset and the tool is sent to the server with no
        // `parameters`, which makes the model emit invalid tool-call arguments.
        .input_schema = R"({"type":"object","properties":{}})",
        .execute_body = R"JS(
          // The Inbox nav link always points at the #inbox view, so match on
          // that rather than on Gmail's volatile CSS class names. Its
          // aria-label reads e.g. "Inbox 2112 unread", which is exactly the
          // information we want to surface.
          const inboxLink = document.querySelector('a[href*="#inbox"]');
          if (!inboxLink) {
            return 'Could not find the Inbox link on this page.';
          }
          return inboxLink.getAttribute('aria-label') || 'Inbox 0 unread';
        )JS",
    },
    // Generic example used for easy local verification on example.com.
    {
        .url_pattern = "https://example.com/*",
        .tool_name = "page_heading",
        .description =
            "Return the text of the first top-level heading (<h1>) on the "
            "current page.",
        .input_schema = R"({"type":"object","properties":{}})",
        .execute_body = R"JS(
          const h1 = document.querySelector('h1');
          return h1 ? h1.textContent.trim() : 'No <h1> found on this page.';
        )JS",
    },
};

}  // namespace

base::span<const WebMcpInjectionRule> GetWebMcpInjectionRules() {
  return base::span(kRules);
}

}  // namespace ai_chat
