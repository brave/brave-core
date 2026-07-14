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

TEST(WebMcpInjectionRuleTest, RejectsMalformedFences) {
  // Opening fence but no closing fence.
  EXPECT_FALSE(WebMcpInjectionRule::ParseScript(R"(// ==WebMCP==
// @name tool
// @match https://example.com/*
// @description d.
return 'ok';
)"));

  // Closing fence but no opening fence.
  EXPECT_FALSE(WebMcpInjectionRule::ParseScript(R"(// @name tool
// @match https://example.com/*
// ==/WebMCP==
return 'ok';
)"));

  // Fences in the wrong order (close before open).
  EXPECT_FALSE(WebMcpInjectionRule::ParseScript(R"(// ==/WebMCP==
// @name tool
// @match https://example.com/*
// ==WebMCP==
return 'ok';
)"));

  // Trailing text on the fence line means it is not recognized as a fence
  // (the stripped line must equal the marker exactly).
  EXPECT_FALSE(WebMcpInjectionRule::ParseScript(R"(// ==WebMCP== v2
// @name tool
// @match https://example.com/*
// ==/WebMCP==
return 'ok';
)"));

  // Fence markers are case-sensitive.
  EXPECT_FALSE(WebMcpInjectionRule::ParseScript(R"(// ==webmcp==
// @name tool
// @match https://example.com/*
// ==/webmcp==
return 'ok';
)"));

  // Well-formed fences but an empty metadata block.
  EXPECT_FALSE(WebMcpInjectionRule::ParseScript(R"(// ==WebMCP==
// ==/WebMCP==
return 'ok';
)"));
}

TEST(WebMcpInjectionRuleTest, RejectsMetadataKeyWithNoValue) {
  // `@name` with no value (or a value that is only whitespace, which is trimmed
  // away) leaves the name unset, so the rule is rejected even though the key is
  // present.
  EXPECT_FALSE(WebMcpInjectionRule::ParseScript(R"(// ==WebMCP==
// @name
// @match https://example.com/*
// @description d.
// ==/WebMCP==
return 'ok';
)"));

  // `@match` with no value leaves the pattern list empty.
  EXPECT_FALSE(WebMcpInjectionRule::ParseScript(R"(// ==WebMCP==
// @name tool
// @match
// @description d.
// ==/WebMCP==
return 'ok';
)"));
}

TEST(WebMcpInjectionRuleTest, IgnoresUnknownKeysAndNonMetadataLines) {
  constexpr char kScript[] = R"(// ==WebMCP==
// Free-form comment describing the tool.
// @name tool
// @match https://example.com/*
// @description d.
// @unknown whatever
// @run-at document-end
// not an @ line either
// ==/WebMCP==
return 'ok';
)";

  auto rule = WebMcpInjectionRule::ParseScript(kScript);
  ASSERT_TRUE(rule);
  EXPECT_EQ(rule->tool_name, "tool");
  ASSERT_EQ(rule->url_patterns.size(), 1u);
}

TEST(WebMcpInjectionRuleTest, LastValueWinsForSingleValueKeys) {
  constexpr char kScript[] = R"(// ==WebMCP==
// @name first
// @name second
// @match https://example.com/*
// @description d.
// @schema {"type":"object","properties":{"a":{}}}
// @schema {"type":"object","properties":{"b":{}}}
// ==/WebMCP==
return 'ok';
)";

  auto rule = WebMcpInjectionRule::ParseScript(kScript);
  ASSERT_TRUE(rule);
  EXPECT_EQ(rule->tool_name, "second");
  EXPECT_EQ(rule->input_schema,
            R"({"type":"object","properties":{"b":{}}})");
}

TEST(WebMcpInjectionRuleTest, DoesNotValidateSchemaJson) {
  // ParseScript stores `@schema` verbatim and does not check that it is valid
  // JSON. A malformed schema is accepted as-is (validation, if any, happens
  // downstream). This test documents that behavior so a future change that
  // adds validation is a deliberate one.
  constexpr char kScript[] = R"(// ==WebMCP==
// @name tool
// @match https://example.com/*
// @description d.
// @schema this-is-not-json
// ==/WebMCP==
return 'ok';
)";

  auto rule = WebMcpInjectionRule::ParseScript(kScript);
  ASSERT_TRUE(rule);
  EXPECT_EQ(rule->input_schema, "this-is-not-json");
}

TEST(WebMcpInjectionRuleTest, ToleratesIndentationAndMissingCommentMarkers) {
  // Leading whitespace before the `//` and fence lines without a `//` marker
  // are both tolerated (the marker is stripped and the line trimmed before the
  // fence comparison).
  constexpr char kScript[] = R"(  // ==WebMCP==
  // @name tool
  // @match https://example.com/*
  // @description d.
==/WebMCP==
return 'ok';
)";

  auto rule = WebMcpInjectionRule::ParseScript(kScript);
  ASSERT_TRUE(rule);
  EXPECT_EQ(rule->tool_name, "tool");
}

TEST(WebMcpInjectionRuleTest, ToleratesTabSeparatedEntriesAndCrlf) {
  // Tab between `@key` and value, and Windows CRLF line endings.
  constexpr char kScript[] =
      "// ==WebMCP==\r\n"
      "// @name\ttool\r\n"
      "// @match\thttps://example.com/*\r\n"
      "// @description\td.\r\n"
      "// ==/WebMCP==\r\n"
      "return 'ok';\r\n";

  auto rule = WebMcpInjectionRule::ParseScript(kScript);
  ASSERT_TRUE(rule);
  EXPECT_EQ(rule->tool_name, "tool");
  ASSERT_EQ(rule->url_patterns.size(), 1u);
  EXPECT_EQ(rule->url_patterns[0], "https://example.com/*");
}

}  // namespace web_mcp
