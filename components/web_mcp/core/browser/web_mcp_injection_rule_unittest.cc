// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/web_mcp/core/browser/web_mcp_injection_rule.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace web_mcp {

TEST(WebMcpInjectionRuleTest, ParsesFullScript) {
  constexpr char kScript[] = R"(// Copyright header.

// ==WebMCP==
// @name        unread_count
// @match       https://mail.google.com/*
// @description Report the number of unread messages
// @description in the Gmail inbox.
// @schema      {"type":"object","properties":{}}
// ==/WebMCP==

const inboxLink = document.querySelector('a[href*="#inbox"]');
return inboxLink ? inboxLink.getAttribute('aria-label') : 'not found';
)";

  auto rule = WebMcpInjectionRule::ParseScript(kScript);
  ASSERT_TRUE(rule);
  EXPECT_EQ(rule->tool_name, "unread_count");
  // Multiple @description lines are joined with a single space.
  EXPECT_EQ(rule->description,
            "Report the number of unread messages in the Gmail inbox.");
  EXPECT_EQ(rule->input_schema, R"({"type":"object","properties":{}})");
  ASSERT_EQ(rule->url_patterns.size(), 1u);
  EXPECT_EQ(rule->url_patterns[0], "https://mail.google.com/*");
  // The body starts after the closing fence and is trimmed of blank lines.
  EXPECT_TRUE(rule->execute_body.starts_with("const inboxLink"));
  EXPECT_TRUE(rule->execute_body.ends_with("'not found';"));
}

TEST(WebMcpInjectionRuleTest, DefaultsInputSchemaWhenOmitted) {
  constexpr char kScript[] = R"(// ==WebMCP==
// @name  page_heading
// @match https://example.com/*
// @description Return the heading.
// ==/WebMCP==
return document.querySelector('h1')?.textContent ?? '';
)";

  auto rule = WebMcpInjectionRule::ParseScript(kScript);
  ASSERT_TRUE(rule);
  EXPECT_EQ(rule->input_schema, R"({"type":"object","properties":{}})");
}

TEST(WebMcpInjectionRuleTest, SupportsMultipleMatchPatterns) {
  constexpr char kScript[] = R"(// ==WebMCP==
// @name  multi
// @match https://a.example.com/*
// @match https://b.example.com/*
// @description tool.
// ==/WebMCP==
return 'ok';
)";

  auto rule = WebMcpInjectionRule::ParseScript(kScript);
  ASSERT_TRUE(rule);
  ASSERT_EQ(rule->url_patterns.size(), 2u);
  EXPECT_TRUE(rule->Matches(GURL("https://a.example.com/inbox")));
  EXPECT_TRUE(rule->Matches(GURL("https://b.example.com/")));
  EXPECT_FALSE(rule->Matches(GURL("https://c.example.com/")));
}

TEST(WebMcpInjectionRuleTest, RejectsScriptWithoutMetadataBlock) {
  EXPECT_FALSE(WebMcpInjectionRule::ParseScript("return 'ok';"));
}

TEST(WebMcpInjectionRuleTest, RejectsScriptMissingRequiredFields) {
  // Missing @name.
  EXPECT_FALSE(WebMcpInjectionRule::ParseScript(R"(// ==WebMCP==
// @match https://example.com/*
// ==/WebMCP==
return 'ok';
)"));

  // Missing @match.
  EXPECT_FALSE(WebMcpInjectionRule::ParseScript(R"(// ==WebMCP==
// @name tool
// ==/WebMCP==
return 'ok';
)"));

  // Missing body.
  EXPECT_FALSE(WebMcpInjectionRule::ParseScript(R"(// ==WebMCP==
// @name tool
// @match https://example.com/*
// ==/WebMCP==
)"));
}

}  // namespace web_mcp
