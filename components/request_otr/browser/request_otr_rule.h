/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_REQUEST_OTR_BROWSER_REQUEST_OTR_RULE_H_
#define BRAVE_COMPONENTS_REQUEST_OTR_BROWSER_REQUEST_OTR_RULE_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/flat_set.h"
#include "base/json/json_value_converter.h"
#include "base/types/expected.h"
#include "base/values.h"
#include "components/prefs/pref_service.h"
#include "extensions/common/url_pattern_set.h"

class GURL;

namespace request_otr {

class RequestOTRRule {
 public:
  RequestOTRRule();
  ~RequestOTRRule();

  // Registers the mapping between JSON field names and the members in this
  // class.
  static void RegisterJSONConverter(
      base::JSONValueConverter<RequestOTRRule>* converter);
  static base::expected<std::pair<std::vector<std::unique_ptr<RequestOTRRule>>,
                                  base::flat_set<std::string>>,
                        std::string>
  ParseRules(const std::string& contents);
  static const std::string GetETLDForRequestOTR(const std::string& host);
  static bool GetURLPatternSetFromValue(const base::Value* value,
                                        extensions::URLPatternSet* result);

  bool ShouldBlock(const GURL& url) const;
  const extensions::URLPatternSet& include_pattern_set() const {
    return include_pattern_set_;
  }

 private:
  extensions::URLPatternSet include_pattern_set_;
  extensions::URLPatternSet exclude_pattern_set_;
};

}  // namespace request_otr

#endif  // BRAVE_COMPONENTS_REQUEST_OTR_BROWSER_REQUEST_OTR_RULE_H_
