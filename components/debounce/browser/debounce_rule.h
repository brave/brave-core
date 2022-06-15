/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_DEBOUNCE_BROWSER_DEBOUNCE_RULE_H_
#define BRAVE_COMPONENTS_DEBOUNCE_BROWSER_DEBOUNCE_RULE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/containers/flat_set.h"
#include "base/json/json_value_converter.h"
#include "base/values.h"
#include "components/prefs/pref_service.h"
#include "extensions/common/url_pattern_set.h"

class GURL;

namespace debounce {

enum DebounceAction {
  kDebounceNoAction,
  kDebounceRedirectToParam,
  kDebounceRegexPath,
  kDebounceBase64DecodeAndRedirectToParam
};

enum DebouncePrependScheme {
  kDebounceNoSchemePrepend,
  kDebounceSchemePrependHttp,
  kDebounceSchemePrependHttps
};

class DebounceRule {
 public:
  DebounceRule();
  ~DebounceRule();

  // Registers the mapping between JSON field names and the members in this
  // class.
  static void RegisterJSONConverter(
      base::JSONValueConverter<DebounceRule>* converter);
  static bool ParseDebounceAction(base::StringPiece value,
                                  DebounceAction* field);
  static bool ParsePrependScheme(base::StringPiece value,
                                 DebouncePrependScheme* field);
  static void ParseRules(base::Value::List root,
                         std::vector<std::unique_ptr<DebounceRule>>* rules,
                         base::flat_set<std::string>* host_cache);
  static const std::string GetETLDForDebounce(const std::string& host);
  static bool GetURLPatternSetFromValue(const base::Value* value,
                                        extensions::URLPatternSet* result);

  bool Apply(const GURL& original_url,
             GURL* final_url,
             const PrefService* prefs) const;
  const extensions::URLPatternSet& include_pattern_set() const {
    return include_pattern_set_;
  }

 private:
  bool CheckPrefForRule(const PrefService* prefs) const;
  extensions::URLPatternSet include_pattern_set_;
  extensions::URLPatternSet exclude_pattern_set_;
  DebounceAction action_;
  DebouncePrependScheme prepend_scheme_;
  std::string param_;
  std::string pref_;
};

}  // namespace debounce

#endif  // BRAVE_COMPONENTS_DEBOUNCE_BROWSER_DEBOUNCE_RULE_H_
