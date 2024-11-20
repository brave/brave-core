// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_YOUTUBE_SCRIPT_INJECTOR_BROWSER_CORE_YOUTUBE_RULE_H_
#define BRAVE_COMPONENTS_YOUTUBE_SCRIPT_INJECTOR_BROWSER_CORE_YOUTUBE_RULE_H_

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

namespace youtube_script_injector {

// Holds the loaded script text when a rule is matched.
struct MatchedRule {
  std::string policy_script;
  int version;
};

// Format of the youtube.json file:
// [
//   {
//     "include": [
//       "https://twitter.com/*"
//     ],
//     "exclude": [
//     ],
//     "version": 1,
//     "policy_script": "twitter/policy.js"
//   }, ...
// ]
// Note that "policy_script" gives a path
// relative to the component under scripts/
// This class describes a single rule in the youtube.json file.
class YouTubeRule {
 public:
  YouTubeRule();
  ~YouTubeRule();
  YouTubeRule(const YouTubeRule& other);  // needed for std::vector<YouTubeRule>

  // Registers the mapping between JSON field names and the members in this
  // class.
  static void RegisterJSONConverter(
      base::JSONValueConverter<YouTubeRule>* converter);

  // Parse the youtube.json file contents into a vector of YouTubeRule.
  static std::optional<std::vector<YouTubeRule>> ParseRules(
      const std::string& contents);
  // Check if this rule matches the given URL.
  // bool ShouldInsertScript(const GURL& url) const;
  bool IsYouTubeDomain(const GURL& url) const;

  // Getters.
  const base::FilePath& GetPolicyScript() const { return policy_script_path_; }
  int GetVersion() const { return version_; }

 private:
  // extensions::URLPatternSet include_pattern_set_;
  // extensions::URLPatternSet exclude_pattern_set_;
  // This is a path (not content) relative to the component under scripts/.
  base::FilePath policy_script_path_;
  // Used for checking if the last inserted script is the latest version.
  int version_;
};

}  // namespace youtube_script_injector

#endif  // BRAVE_COMPONENTS_YOUTUBE_SCRIPT_INJECTOR_BROWSER_CORE_YOUTUBE_RULE_H_
