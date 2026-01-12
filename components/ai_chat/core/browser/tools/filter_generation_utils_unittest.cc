// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/tools/filter_generation_utils.h"

#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

TEST(FilterGenerationUtilsTest, BuildFilterGenerationPrompt) {
  std::string page_content = "Cookie banner text";
  std::string dom_structure = R"([{"tag":"div","classes":["banner"],"id":"","text":"Accept cookies"}])";
  std::string target_description = "cookie banner";
  std::string page_url = "https://example.com/page";

  std::string prompt = BuildFilterGenerationPrompt(
      page_content, dom_structure, target_description, page_url);

  EXPECT_FALSE(prompt.empty());
  EXPECT_NE(prompt.find("example.com"), std::string::npos);
  EXPECT_NE(prompt.find("cookie banner"), std::string::npos);
  EXPECT_NE(prompt.find("DOM Structure"), std::string::npos);
}

TEST(FilterGenerationUtilsTest, ParseMarkdownFilterResponse_CSS) {
  std::string markdown = R"(
```css
.cookie-banner
```
**Filter Rule:** example.com##.cookie-banner
**Description:** Hides cookie banner
**Confidence:** 85
**Reasoning:** Target common cookie banner class
**Target Elements:** ["banner"]
)";

  auto filter = ParseMarkdownFilterResponse(markdown);

  ASSERT_TRUE(filter.has_value());
  EXPECT_EQ(filter->type, mojom::GeneratedFilterType::CSS_SELECTOR);
  EXPECT_EQ(filter->code, "\n.cookie-banner\n");
  EXPECT_EQ(filter->filter_rule, "example.com##.cookie-banner");
  EXPECT_EQ(filter->description, "Hides cookie banner");
  EXPECT_EQ(filter->confidence, 85);
  EXPECT_EQ(filter->reasoning, "Target common cookie banner class");
}

TEST(FilterGenerationUtilsTest, ParseMarkdownFilterResponse_Scriptlet) {
  std::string markdown = R"(
```javascript
(function() {
  document.querySelector('.banner').remove();
})();
```
**Scriptlet Name:** user-hide-banner-example-com
**Filter Rule:** example.com##+js(user-hide-banner-example-com)
**Description:** Removes banner element
**Confidence:** 90
**Reasoning:** Direct element removal
)";

  auto filter = ParseMarkdownFilterResponse(markdown);

  ASSERT_TRUE(filter.has_value());
  EXPECT_EQ(filter->type, mojom::GeneratedFilterType::SCRIPTLET);
  EXPECT_NE(filter->code.find("querySelector"), std::string::npos);
  EXPECT_EQ(filter->filter_rule, "example.com##+js(user-hide-banner-example-com)");
  EXPECT_EQ(filter->description, "Removes banner element");
  EXPECT_EQ(filter->confidence, 90);
  EXPECT_TRUE(filter->scriptlet_name.has_value());
  EXPECT_EQ(filter->scriptlet_name.value(), "user-hide-banner-example-com.js");
}

TEST(FilterGenerationUtilsTest, ParseMarkdownFilterResponse_Invalid) {
  std::string markdown = "No code blocks here";

  auto filter = ParseMarkdownFilterResponse(markdown);

  EXPECT_FALSE(filter.has_value());
}

TEST(FilterGenerationUtilsTest, ValidateFilterCode_SafeJS) {
  std::string safe_code = R"(
(function() {
  const element = document.querySelector('.banner');
  if (element) {
    element.remove();
  }
})();
)";

  EXPECT_TRUE(ValidateFilterCode(safe_code, mojom::GeneratedFilterType::SCRIPTLET));
}

TEST(FilterGenerationUtilsTest, ValidateFilterCode_DangerousJS) {
  std::string dangerous_code = R"(
(function() {
  eval('malicious code');
})();
)";

  EXPECT_FALSE(ValidateFilterCode(dangerous_code, mojom::GeneratedFilterType::SCRIPTLET));
}

TEST(FilterGenerationUtilsTest, ValidateFilterCode_DangerousJS_InnerHTML) {
  std::string dangerous_code = R"(
(function() {
  document.body.innerHTML = 'bad';
})();
)";

  EXPECT_FALSE(ValidateFilterCode(dangerous_code, mojom::GeneratedFilterType::SCRIPTLET));
}

TEST(FilterGenerationUtilsTest, ValidateFilterCode_CSS) {
  std::string css_code = ".cookie-banner { display: none; }";

  EXPECT_TRUE(ValidateFilterCode(css_code, mojom::GeneratedFilterType::CSS_SELECTOR));
}

TEST(FilterGenerationUtilsTest, ValidateFilterCode_DangerousCSS) {
  std::string dangerous_css = "body { background: url(javascript:alert(1)); }";

  EXPECT_FALSE(ValidateFilterCode(dangerous_css, mojom::GeneratedFilterType::CSS_SELECTOR));
}

TEST(FilterGenerationUtilsTest, GenerateSafeScriptletName) {
  std::string name = GenerateSafeScriptletName("hide cookie banner", "example.com");

  EXPECT_FALSE(name.empty());
  EXPECT_EQ(name.substr(0, 5), "user-");
  EXPECT_NE(name.find("hide"), std::string::npos);
  EXPECT_NE(name.find("example"), std::string::npos);
  EXPECT_NE(name.find(".js"), std::string::npos);
}

TEST(FilterGenerationUtilsTest, GenerateSafeScriptletName_Sanitization) {
  std::string name = GenerateSafeScriptletName("test@#$%", "site!.com");

  EXPECT_FALSE(name.empty());
  EXPECT_EQ(name.find('@'), std::string::npos);
  EXPECT_EQ(name.find('!'), std::string::npos);
  EXPECT_NE(name.find("test"), std::string::npos);
  EXPECT_NE(name.find("site"), std::string::npos);
}

}  // namespace ai_chat
