/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_PATTERNS_V2_H_
#define BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_PATTERNS_V2_H_

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "brave/components/web_discovery/browser/relevant_site.h"
#include "brave/components/web_discovery/browser/value_transform.h"

namespace web_discovery {

// Represents a single extraction rule within a selector
struct V2ExtractionRule {
  V2ExtractionRule();
  ~V2ExtractionRule();

  V2ExtractionRule(const V2ExtractionRule&) = delete;
  V2ExtractionRule& operator=(const V2ExtractionRule&) = delete;

  V2ExtractionRule(V2ExtractionRule&&);
  V2ExtractionRule& operator=(V2ExtractionRule&&);

  // Optional sub-selector for nested elements
  std::optional<std::string> sub_selector;
  // Attribute to extract (e.g., "textContent", "href")
  std::string attribute;
  // Transformation functions to apply to the extracted value
  std::vector<std::unique_ptr<ValueTransform>> transforms;
};

// Represents an input group with its extraction rules
struct V2InputGroup {
  V2InputGroup();
  ~V2InputGroup();

  V2InputGroup(const V2InputGroup&) = delete;
  V2InputGroup& operator=(const V2InputGroup&) = delete;

  V2InputGroup(V2InputGroup&&);
  V2InputGroup& operator=(V2InputGroup&&);

  // Whether to extract from all matching elements (true) or just the first
  // (false)
  bool select_all;
  // Map of field names to extraction rules (supports multiple rules via
  // firstMatch)
  base::flat_map<std::string, std::vector<V2ExtractionRule>> extraction_rules;
};

// Represents an output field definition
struct V2OutputField {
  V2OutputField();
  ~V2OutputField();

  V2OutputField(const V2OutputField&) = delete;
  V2OutputField& operator=(const V2OutputField&) = delete;

  V2OutputField(V2OutputField&&);
  V2OutputField& operator=(V2OutputField&&);

  // The field key/name
  std::string key;
  // Source selector (if specified)
  std::optional<std::string> source_selector;
  // Required keys for validation
  std::vector<std::string> required_keys;
  // Whether this field is optional
  bool optional = false;
};

// Represents an output group definition
struct V2OutputGroup {
  V2OutputGroup();
  ~V2OutputGroup();

  V2OutputGroup(const V2OutputGroup&) = delete;
  V2OutputGroup& operator=(const V2OutputGroup&) = delete;

  V2OutputGroup(V2OutputGroup&&);
  V2OutputGroup& operator=(V2OutputGroup&&);

  // The output group name/key
  std::string action;
  // List of fields in this output group
  std::vector<V2OutputField> fields;
};

// Represents a complete site pattern configuration
struct V2SitePattern {
  V2SitePattern();
  ~V2SitePattern();

  V2SitePattern(const V2SitePattern&) = delete;
  V2SitePattern& operator=(const V2SitePattern&) = delete;

  V2SitePattern(V2SitePattern&&);
  V2SitePattern& operator=(V2SitePattern&&);

  // Map of CSS selectors to input groups (input section)
  base::flat_map<std::string, V2InputGroup> input_groups;
  // Vector of output groups (output section)
  std::vector<V2OutputGroup> output_groups;
};

// The complete v2 patterns configuration
struct V2PatternsGroup {
  V2PatternsGroup();
  ~V2PatternsGroup();

  V2PatternsGroup(const V2PatternsGroup&) = delete;
  V2PatternsGroup& operator=(const V2PatternsGroup&) = delete;

  V2PatternsGroup(V2PatternsGroup&&);
  V2PatternsGroup& operator=(V2PatternsGroup&&);

  // Map of RelevantSite to their patterns
  base::flat_map<RelevantSite, V2SitePattern> site_patterns;
};

// Parses v2 patterns JSON. Returns nullptr if parsing fails.
std::unique_ptr<V2PatternsGroup> ParseV2Patterns(
    std::string_view patterns_json);

}  // namespace web_discovery

#endif  // BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_PATTERNS_V2_H_
