// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_YOUTUBE_SCRIPT_INJECTOR_BROWSER_CORE_YOUTUBE_RULE_REGISTRY_H_
#define BRAVE_COMPONENTS_YOUTUBE_SCRIPT_INJECTOR_BROWSER_CORE_YOUTUBE_RULE_REGISTRY_H_

#include <memory>
#include <string>
#include <vector>

#include "base/component_export.h"
#include "base/gtest_prod_util.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/singleton.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/youtube_script_injector/browser/core/youtube_rule.h"

class GURL;

namespace youtube_script_injector {
// Needed for testing private methods in YouTubeTabHelperBrowserTest.
FORWARD_DECLARE_TEST(YouTubeTabHelperBrowserTest, NoMatch);
FORWARD_DECLARE_TEST(YouTubeTabHelperBrowserTest, RuleMatchTestScriptFalse);
FORWARD_DECLARE_TEST(YouTubeTabHelperBrowserTest, RuleMatchTestScriptTrue);

// This class loads and stores the rules from the youtube.json file.
// It is also used for matching based on the URL.
class COMPONENT_EXPORT(YOUTUBE_SCRIPT_INJECTOR_BROWSER_CORE) YouTubeRuleRegistry {
 public:
  YouTubeRuleRegistry(const YouTubeRuleRegistry&) = delete;
  YouTubeRuleRegistry& operator=(const YouTubeRuleRegistry&) = delete;
  ~YouTubeRuleRegistry();
  static YouTubeRuleRegistry* GetInstance();  // singleton
  // Returns the matched YouTube script injector rule, if any.
  void CheckIfMatch(const GURL& url,
                    base::OnceCallback<void(MatchedRule)> cb) const;
  // Given a path to youtube.json, loads the rules from the file into memory.
  void LoadRules(const base::FilePath& path);

 private:
  YouTubeRuleRegistry();

  // These methods are also called by YouTubeTabHelperBrowserTest.
  // Given contents of youtube.json, loads the rules from the file into memory.
  // Called by |LoadRules| after the file is read.
  void OnLoadRules(const std::string& data);
  // Sets the component path used to resolve the paths to the scripts.
  void SetComponentPath(const base::FilePath& path);

  base::FilePath component_path_;
  std::optional<YouTubeRule> rule_;

  base::WeakPtrFactory<YouTubeRuleRegistry> weak_factory_{this};

  // Needed for testing private methods in YouTubeTabHelperBrowserTest.
  FRIEND_TEST_ALL_PREFIXES(YouTubeTabHelperBrowserTest, NoMatch);
  FRIEND_TEST_ALL_PREFIXES(YouTubeTabHelperBrowserTest, RuleMatchTestScriptFalse);
  FRIEND_TEST_ALL_PREFIXES(YouTubeTabHelperBrowserTest, RuleMatchTestScriptTrue);
  friend class YouTubeTabHelperBrowserTest;

  friend struct base::DefaultSingletonTraits<YouTubeRuleRegistry>;
};

}  // namespace youtube_script_injector

#endif  // BRAVE_COMPONENTS_YOUTUBE_SCRIPT_INJECTOR_BROWSER_CORE_YOUTUBE_RULE_REGISTRY_H_
