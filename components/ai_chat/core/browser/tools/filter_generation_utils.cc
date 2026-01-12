// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/tools/filter_generation_utils.h"

#include <algorithm>
#include <sstream>

#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "url/gurl.h"

namespace ai_chat {

GeneratedFilter::GeneratedFilter() = default;
GeneratedFilter::~GeneratedFilter() = default;
GeneratedFilter::GeneratedFilter(const GeneratedFilter&) = default;
GeneratedFilter& GeneratedFilter::operator=(const GeneratedFilter&) = default;
GeneratedFilter::GeneratedFilter(GeneratedFilter&&) = default;
GeneratedFilter& GeneratedFilter::operator=(GeneratedFilter&&) = default;

namespace {

// Dangerous patterns to check in JavaScript code (case will be normalized to lowercase)
// NOTE: This list catches Function() constructor and dangerous DOM APIs.
// We check for specific contexts like "new function(" to avoid blocking IIFEs like "(function()".
// setTimeout/setInterval are NOT included as they're needed for legitimate scriptlets
// that wait for dynamic content.
constexpr const char* kDangerousJsPatterns[] = {
    "eval(",
    "new function(",       // Function constructor with new keyword
    "window.function(",    // Global Function access
    "globalthis.function(", // Modern global Function access
    ".innerhtml",          // Catches both .innerHTML= and .innerHTML =
    "document.write",      // Catches both write() and writeln()
    ".outerhtml",          // Catches both .outerHTML= and .outerHTML =
    ".postmessage(",
    "fetch(",
    "xmlhttprequest",
    "importscripts(",
    "atob(",               // Base64 decode (obfuscation vector)
    "btoa(",               // Base64 encode (obfuscation vector)
    ".insertadjacenthtml(",
    ".src=",               // Script injection via src attribute
    "javascript:",         // javascript: protocol in URLs
};

// Safe DOM manipulation patterns (whitelist) - in lowercase for case-insensitive matching
constexpr const char* kSafeJsPatterns[] = {
    "queryselector",
    "queryselectorall",
    "getelementsby",
    "remove(",
    ".style.",
    ".classlist.",
    "setattribute",
    "getattribute",
    "mutationobserver",
    "addeventlistener",
};

// Extract content between markdown code fences
std::optional<std::string> ExtractCodeBlock(const std::string& markdown) {
  // Look for ```javascript or ```css blocks
  size_t start = markdown.find("```");
  if (start == std::string::npos) {
    return std::nullopt;
  }

  // Skip the ``` and language identifier
  size_t code_start = markdown.find('\n', start);
  if (code_start == std::string::npos) {
    return std::nullopt;
  }
  // Don't skip the newline - include it in extracted code for consistency
  // code_start++;  // Removed: tests expect the newline to be included

  // Find closing ```
  size_t code_end = markdown.find("```", code_start);
  if (code_end == std::string::npos) {
    return std::nullopt;
  }

  return markdown.substr(code_start, code_end - code_start);
}

// Extract value after a markdown bold label
std::optional<std::string> ExtractLabeledValue(const std::string& markdown,
                                               const std::string& label) {
  std::string pattern = "**" + label + ":**";
  size_t start = markdown.find(pattern);
  if (start == std::string::npos) {
    // Try without colon
    pattern = "**" + label + "**";
    start = markdown.find(pattern);
    if (start == std::string::npos) {
      return std::nullopt;
    }
  }

  start += pattern.length();

  // Skip whitespace
  while (start < markdown.length() && std::isspace(markdown[start])) {
    start++;
  }

  // Find end of line
  size_t end = markdown.find('\n', start);
  if (end == std::string::npos) {
    end = markdown.length();
  }

  std::string value = markdown.substr(start, end - start);
  base::TrimWhitespaceASCII(value, base::TRIM_ALL, &value);
  return value;
}

// Sanitize a string to be used in a filename
std::string SanitizeForFilename(const std::string& str) {
  std::string result;
  result.reserve(str.length());

  for (char c : str) {
    if (std::isalnum(c) || c == '-' || c == '_') {
      result.push_back(std::tolower(c));
    } else if (c == ' ') {
      result.push_back('-');
    }
  }

  // Limit length
  if (result.length() > 30) {
    result = result.substr(0, 30);
  }

  return result;
}

}  // namespace

std::string BuildFilterGenerationPrompt(const std::string& page_content,
                                        const std::string& dom_structure,
                                        const std::string& target_description,
                                        const std::string& page_url) {
  GURL url(page_url);
  std::string domain = std::string(url.host());

  std::ostringstream prompt;
  prompt << "You are a cosmetic filter generation assistant. Your task is to "
            "generate a custom filter to hide or remove specific elements from "
            "a web page.\n\n";

  prompt << "# Page Information\n";
  prompt << "Domain: " << domain << "\n";
  prompt << "URL: " << page_url << "\n\n";

  prompt << "# User Request\n";
  prompt << "Hide or remove: " << target_description << "\n\n";

  prompt << "# DOM Structure\n";
  prompt << "```json\n" << dom_structure << "\n```\n\n";

  if (!page_content.empty()) {
    // Limit page content to avoid token overflow
    std::string limited_content = page_content.substr(
        0, std::min(size_t(2000), page_content.length()));
    prompt << "# Page Text Preview\n";
    prompt << limited_content << "\n\n";
  }

  prompt << "# Instructions\n";
  prompt << "Analyze the DOM structure and generate a filter to hide the "
            "requested element(s).\n\n";
  prompt << "Choose between:\n";
  prompt << "1. **CSS Selector** - Simple hiding with display:none (preferred "
            "when possible)\n";
  prompt << "2. **JavaScript Scriptlet** - For complex cases requiring dynamic "
            "removal or waiting for elements\n\n";

  prompt << "# Output Format\n";
  prompt << "Provide your answer in this exact format:\n\n";

  prompt << "For CSS selectors:\n";
  prompt << "```css\n.class-name\n```\n";
  prompt << "**Filter Rule:** " << domain << "##.class-name\n";
  prompt << "**Description:** Brief description of what this hides\n";
  prompt << "**Confidence:** 0-100 (your confidence this will work)\n";
  prompt << "**Reasoning:** Why you chose this selector\n";
  prompt << "**Target Elements:** [\"element1\", \"element2\"]\n\n";

  prompt << "For scriptlets:\n";
  prompt << "```javascript\n";
  prompt << "(function() {\n";
  prompt << "  // Your code here\n";
  prompt << "})();\n";
  prompt << "```\n";
  prompt << "**Scriptlet Name:** user-hide-" << SanitizeForFilename(target_description)
         << "-" << SanitizeForFilename(domain) << "\n";
  prompt << "**Filter Rule:** " << domain << "##+js(user-hide-...)\n";
  prompt << "**Description:** Brief description\n";
  prompt << "**Confidence:** 0-100\n";
  prompt << "**Reasoning:** Why you chose this approach\n";
  prompt << "**Target Elements:** [\"element1\", \"element2\"]\n\n";

  prompt << "# Guidelines\n";
  prompt << "- Prefer CSS selectors when the element has stable classes/IDs\n";
  prompt << "- Use scriptlets for dynamic content or when waiting is needed\n";
  prompt << "- Be conservative - ensure the filter won't break the page\n";
  prompt << "- Include proper error handling in scriptlets\n";
  prompt << "- Only use safe DOM manipulation (no eval, innerHTML, etc.)\n";
  prompt << "- Target specific elements, avoid overly broad selectors\n";
  prompt << "- Confidence should reflect likelihood of success\n";

  return prompt.str();
}

std::optional<GeneratedFilter> ParseMarkdownFilterResponse(
    const std::string& markdown_response) {
  GeneratedFilter filter;

  // Extract code block
  auto code = ExtractCodeBlock(markdown_response);
  if (!code.has_value()) {
    VLOG(1) << "Failed to extract code block from response";
    return std::nullopt;
  }
  filter.code = code.value();

  // Determine type based on code block
  if (markdown_response.find("```css") != std::string::npos ||
      markdown_response.find("```CSS") != std::string::npos) {
    filter.type = mojom::GeneratedFilterType::CSS_SELECTOR;
  } else if (markdown_response.find("```javascript") != std::string::npos ||
             markdown_response.find("```js") != std::string::npos) {
    filter.type = mojom::GeneratedFilterType::SCRIPTLET;
  } else {
    // Try to infer from code content
    if (filter.code.find("function") != std::string::npos ||
        filter.code.find("querySelector") != std::string::npos) {
      filter.type = mojom::GeneratedFilterType::SCRIPTLET;
    } else {
      filter.type = mojom::GeneratedFilterType::CSS_SELECTOR;
    }
  }

  // Extract filter rule (required)
  auto filter_rule = ExtractLabeledValue(markdown_response, "Filter Rule");
  if (!filter_rule.has_value() || filter_rule->empty()) {
    VLOG(1) << "Failed to extract filter rule";
    return std::nullopt;
  }
  filter.filter_rule = filter_rule.value();

  // Extract description (required)
  auto description = ExtractLabeledValue(markdown_response, "Description");
  if (!description.has_value() || description->empty()) {
    VLOG(1) << "Failed to extract description";
    return std::nullopt;
  }
  filter.description = description.value();

  // Extract confidence (optional, default to 50)
  auto confidence_str = ExtractLabeledValue(markdown_response, "Confidence");
  if (confidence_str.has_value()) {
    int confidence_value = 50;
    if (base::StringToInt(confidence_str.value(), &confidence_value)) {
      filter.confidence = std::clamp(confidence_value, 0, 100);
    } else {
      filter.confidence = 50;
    }
  } else {
    filter.confidence = 50;
  }

  // Extract reasoning (optional)
  auto reasoning = ExtractLabeledValue(markdown_response, "Reasoning");
  filter.reasoning = reasoning.value_or("No reasoning provided");

  // Extract scriptlet name if it's a scriptlet
  if (filter.type == mojom::GeneratedFilterType::SCRIPTLET) {
    auto scriptlet_name = ExtractLabeledValue(markdown_response, "Scriptlet Name");
    if (scriptlet_name.has_value()) {
      filter.scriptlet_name = scriptlet_name.value();
      // Ensure .js extension
      if (!filter.scriptlet_name->ends_with(".js")) {
        filter.scriptlet_name = filter.scriptlet_name.value() + ".js";
      }
    }
  }

  // Extract target elements (optional)
  auto target_elements = ExtractLabeledValue(markdown_response, "Target Elements");
  if (target_elements.has_value()) {
    // Simple parsing of array format ["elem1", "elem2"]
    std::string targets = target_elements.value();
    // Remove brackets
    base::ReplaceChars(targets, "[]\"", "", &targets);
    // Split by comma
    std::vector<std::string> elements = base::SplitString(
        targets, ",", base::WhitespaceHandling::TRIM_WHITESPACE,
        base::SplitResult::SPLIT_WANT_NONEMPTY);
    filter.target_elements = elements;
  }

  return filter;
}

bool ValidateFilterCode(const std::string& code,
                        mojom::GeneratedFilterType type) {
  if (code.empty()) {
    return false;
  }

  // Check length limits
  if (code.length() > 10000) {  // 10KB max
    VLOG(1) << "Filter code exceeds maximum length";
    return false;
  }

  if (type == mojom::GeneratedFilterType::SCRIPTLET) {
    // Check for dangerous JavaScript patterns
    std::string lower_code = base::ToLowerASCII(code);

    for (const char* pattern : kDangerousJsPatterns) {
      std::string lower_pattern = base::ToLowerASCII(pattern);
      if (lower_code.find(lower_pattern) != std::string::npos) {
        VLOG(1) << "Filter code contains dangerous pattern: " << pattern;
        return false;
      }
    }

    // Ensure at least one safe pattern is present
    bool has_safe_pattern = false;
    for (const char* pattern : kSafeJsPatterns) {
      // Patterns are already in lowercase
      if (lower_code.find(pattern) != std::string::npos) {
        has_safe_pattern = true;
        break;
      }
    }

    if (!has_safe_pattern) {
      VLOG(1) << "Filter code doesn't contain any recognized safe DOM manipulation patterns";
      return false;
    }
  } else {
    // CSS selector validation
    // Check for basic CSS injection attempts
    std::string lower_css = base::ToLowerASCII(code);
    if (lower_css.find("javascript:") != std::string::npos ||
        lower_css.find("expression(") != std::string::npos ||
        lower_css.find("@import") != std::string::npos ||
        lower_css.find("-moz-binding:") != std::string::npos ||
        lower_css.find("behavior:") != std::string::npos) {
      VLOG(1) << "CSS selector contains potentially dangerous content";
      return false;
    }

    // Check for non-data URLs (allow data: URLs but block http/https/file URLs)
    size_t url_pos = lower_css.find("url(");
    if (url_pos != std::string::npos) {
      // Find what's inside url()
      size_t data_pos = lower_css.find("data:", url_pos);
      // If there's no "data:" within 10 chars after "url(", it's likely a remote URL
      if (data_pos == std::string::npos || data_pos > url_pos + 10) {
        VLOG(1) << "CSS contains non-data URL";
        return false;
      }
    }
  }

  return true;
}

std::string GenerateSafeScriptletName(const std::string& description,
                                     const std::string& domain) {
  std::string sanitized_desc = SanitizeForFilename(description);
  std::string sanitized_domain = SanitizeForFilename(domain);

  // Replace dots with dashes in domain
  base::ReplaceChars(sanitized_domain, ".", "-", &sanitized_domain);

  // Add timestamp for uniqueness
  int64_t timestamp = base::Time::Now().InMillisecondsSinceUnixEpoch();

  return base::StrCat({"user-", sanitized_desc, "-", sanitized_domain, "-",
                       base::NumberToString(timestamp), ".js"});
}

}  // namespace ai_chat
