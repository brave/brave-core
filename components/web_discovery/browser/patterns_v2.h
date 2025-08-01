/* Copyright (c) 2024 The Brave Authors. All rights reserved.
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
#include "base/values.h"
#include "brave/components/web_discovery/browser/relevant_site.h"

namespace web_discovery {

// Enum for transformation function names used in v2 patterns
enum class PatternsV2TransformFunction {
  kTrySplit,
  kDecodeURIComponent,
  kFilterExact,
  kTryDecodeURIComponent,
  kRemoveParams,
  kMaskU,
  kSplit,
  kTrim,
  kRelaxedMaskU,
  kJson,
  kQueryParam,
  kRequireURL,
};

// Represents a transformation function to be applied to extracted data
struct PatternsV2Transform {
  PatternsV2Transform();
  ~PatternsV2Transform();

  PatternsV2Transform(const PatternsV2Transform&) = delete;
  PatternsV2Transform& operator=(const PatternsV2Transform&) = delete;

  PatternsV2Transform(PatternsV2Transform&&);
  PatternsV2Transform& operator=(PatternsV2Transform&&);

  // The transformation function to apply
  PatternsV2TransformFunction function;
  // Function arguments
  base::Value::List arguments;
};

// Represents a single extraction rule within a selector
struct PatternsV2ExtractionRule {
  PatternsV2ExtractionRule();
  ~PatternsV2ExtractionRule();

  PatternsV2ExtractionRule(const PatternsV2ExtractionRule&) = delete;
  PatternsV2ExtractionRule& operator=(const PatternsV2ExtractionRule&) = delete;

  PatternsV2ExtractionRule(PatternsV2ExtractionRule&&);
  PatternsV2ExtractionRule& operator=(PatternsV2ExtractionRule&&);

  // Optional sub-selector for nested elements
  std::optional<std::string> sub_selector;
  // Attribute to extract (e.g., "textContent", "href")
  std::string attribute;
  // Transformation functions to apply to the extracted value
  std::vector<PatternsV2Transform> transforms;
};

// Represents an input group with its extraction rules
struct PatternsV2InputGroup {
  PatternsV2InputGroup();
  ~PatternsV2InputGroup();

  PatternsV2InputGroup(const PatternsV2InputGroup&) = delete;
  PatternsV2InputGroup& operator=(const PatternsV2InputGroup&) = delete;

  PatternsV2InputGroup(PatternsV2InputGroup&&);
  PatternsV2InputGroup& operator=(PatternsV2InputGroup&&);

  enum class Type {
    kFirst,  // Extract from first matching element
    kAll     // Extract from all matching elements
  };

  // The CSS selector
  std::string selector;
  // Whether to extract from first or all matching elements
  Type type;
  // Map of field names to vectors of extraction rules
  base::flat_map<std::string, std::vector<PatternsV2ExtractionRule>>
      extraction_rules;
};

// Represents an output field definition
struct PatternsV2OutputField {
  PatternsV2OutputField();
  ~PatternsV2OutputField();

  PatternsV2OutputField(const PatternsV2OutputField&) = delete;
  PatternsV2OutputField& operator=(const PatternsV2OutputField&) = delete;

  PatternsV2OutputField(PatternsV2OutputField&&);
  PatternsV2OutputField& operator=(PatternsV2OutputField&&);

  // The field key/name
  std::string key;
  // Source selector (if specified)
  std::optional<std::string> source;
  // Required keys for validation
  std::vector<std::string> required_keys;
  // Whether this field is optional
  bool optional = false;
};

// Represents an output group definition
struct PatternsV2OutputGroup {
  PatternsV2OutputGroup();
  ~PatternsV2OutputGroup();

  PatternsV2OutputGroup(const PatternsV2OutputGroup&) = delete;
  PatternsV2OutputGroup& operator=(const PatternsV2OutputGroup&) = delete;

  PatternsV2OutputGroup(PatternsV2OutputGroup&&);
  PatternsV2OutputGroup& operator=(PatternsV2OutputGroup&&);

  // The output group name/key
  std::string name;
  // List of fields in this output group
  std::vector<PatternsV2OutputField> fields;
};

// Represents a complete site pattern configuration
struct PatternsV2SitePattern {
  PatternsV2SitePattern();
  ~PatternsV2SitePattern();

  PatternsV2SitePattern(const PatternsV2SitePattern&) = delete;
  PatternsV2SitePattern& operator=(const PatternsV2SitePattern&) = delete;

  PatternsV2SitePattern(PatternsV2SitePattern&&);
  PatternsV2SitePattern& operator=(PatternsV2SitePattern&&);

  // Vector of input groups (input section)
  std::vector<PatternsV2InputGroup> input_groups;
  // Vector of output groups (output section)
  std::vector<PatternsV2OutputGroup> output_groups;
};

// The complete v2 patterns configuration
struct PatternsV2PatternsGroup {
  PatternsV2PatternsGroup();
  ~PatternsV2PatternsGroup();

  PatternsV2PatternsGroup(const PatternsV2PatternsGroup&) = delete;
  PatternsV2PatternsGroup& operator=(const PatternsV2PatternsGroup&) = delete;

  PatternsV2PatternsGroup(PatternsV2PatternsGroup&&);
  PatternsV2PatternsGroup& operator=(PatternsV2PatternsGroup&&);

  // Map of RelevantSite to their patterns
  base::flat_map<RelevantSite, PatternsV2SitePattern> site_patterns;
};

// Parses v2 patterns JSON. Returns nullptr if parsing fails.
std::unique_ptr<PatternsV2PatternsGroup> ParseV2Patterns(
    std::string_view patterns_json);

}  // namespace web_discovery

#endif  // BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_PATTERNS_V2_H_
