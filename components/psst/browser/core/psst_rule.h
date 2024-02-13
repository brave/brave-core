// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_PSST_BROWSER_CORE_PSST_RULE_H_
#define BRAVE_COMPONENTS_PSST_BROWSER_CORE_PSST_RULE_H_

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/flat_set.h"
#include "base/files/file_path.h"
#include "base/json/json_value_converter.h"
#include "base/values.h"
#include "extensions/common/url_pattern_set.h"

class GURL;

namespace psst {

// Holds the loaded script text when a rule is matched.
struct MatchedRule {
  std::string test_script;
  std::string policy_script;
  int version;
};

// Format of the psst.json file:
// [
//   {
//     "include": [
//       "https://twitter.com/*"
//     ],
//     "exclude": [
//     ],
//     "version": 1,
//     "test_script": "twitter/test.js",
//     "policy_script": "twitter/policy.js"
//   }, ...
// ]
// Note that "test_script" and "policy_script" give paths
// relative to the component under scripts/
// This class describes a single rule in the psst.json file.
class PsstRule {
 public:
  PsstRule();
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
  const base::FilePath& GetPolicyScript() const { return policy_script_path_; }
  const base::FilePath& GetTestScript() const { return test_script_path_; }
  int GetVersion() const { return version_; }

 private:
  extensions::URLPatternSet include_pattern_set_;
  extensions::URLPatternSet exclude_pattern_set_;
  // These are paths (not contents!) relative to the component under scripts/.
  base::FilePath policy_script_path_;
  base::FilePath test_script_path_;
  // Used for checking if the last inserted script is the latest version.
  int version_;
};

}  // namespace psst

#endif  // BRAVE_COMPONENTS_PSST_BROWSER_CORE_PSST_RULE_H_
