// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_PSST_CORE_BROWSER_PSST_RULE_H_
#define BRAVE_COMPONENTS_PSST_CORE_BROWSER_PSST_RULE_H_

#include <optional>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/json/json_value_converter.h"

class GURL;

namespace psst {

// A simplified URL match pattern parsed from a psst.json "include"/"exclude"
// entry, e.g. "https://twitter.com/*" or "https://*.twitter.com/*". Only the
// https scheme is supported. An optional "*." host prefix matches the domain
// and all of its subdomains. The path supports "*" glob wildcards.
struct PsstUrlPattern {
  // Parses |pattern|. Returns std::nullopt if the pattern is malformed or does
  // not use the https scheme.
  static std::optional<PsstUrlPattern> Create(const std::string& pattern);

  bool Matches(const GURL& url) const;

  std::string scheme;
  std::string host;
  bool match_subdomains = false;
  std::string path;
};

// Format of the psst.json file:
// [
//   {
//     "include": [
//       "https://twitter.com/*"
//     ],
//     "exclude": [
//     ],
//     "name": "twitter",
//     "version": 1,
//     "user_script": "user.js",
//     "policy_script": "policy.js"
//   }, ...
// ]
// Note that values for the "_script" keys give paths
// relative to the component under scripts/<name>/, NOT script contents.

// This class describes a single rule in the psst.json file.
class PsstRule {
 public:
  ~PsstRule();
  PsstRule(const PsstRule& other);  // needed for std::vector<PsstRule>

  // Registers the mapping between JSON field names and the members in this
  // class.
  static void RegisterJSONConverter(
      base::JSONValueConverter<PsstRule>* converter);

  // Parse the psst.json file contents into a vector of PsstRule.
  static std::optional<std::vector<PsstRule>> ParseRules(
      const std::string& contents);
  // Check if this rule matches the given URL.
  bool ShouldInsertScript(const GURL& url) const;

  // Getters.
  const std::string& name() const { return name_; }
  const base::FilePath& policy_script_path() const {
    return policy_script_path_;
  }
  const base::FilePath& user_script_path() const { return user_script_path_; }
  int version() const { return version_; }

 private:
  PsstRule();
  std::vector<PsstUrlPattern> include_patterns_;
  std::vector<PsstUrlPattern> exclude_patterns_;
  std::string name_;
  // These are paths (not contents!) relative to the component under scripts/.
  base::FilePath policy_script_path_;
  base::FilePath user_script_path_;
  // Used for checking if the last inserted script is the latest version.
  int version_;
};

}  // namespace psst

#endif  // BRAVE_COMPONENTS_PSST_CORE_BROWSER_PSST_RULE_H_
