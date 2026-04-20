// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_QUERY_FILTER_BROWSER_QUERY_FILTER_DATA_H_
#define BRAVE_COMPONENTS_QUERY_FILTER_BROWSER_QUERY_FILTER_DATA_H_

#include <string>
#include <string_view>
#include <vector>

#include "base/no_destructor.h"
#include "base/version.h"

namespace query_filter {

struct QueryFilterRule {
  QueryFilterRule();
  ~QueryFilterRule();

  QueryFilterRule(const QueryFilterRule&);
  QueryFilterRule& operator=(const QueryFilterRule&);

  std::vector<std::string> include;
  std::vector<std::string> exclude;
  std::vector<std::string> params;
};

// Singleton responsible for storing rules from the query filter component.
// See brave/adblock-lists/brave-lists/query-filter.json
class QueryFilterData {
 public:
  // Returns a singleton instance of QueryFilterData.
  // Returns nullptr if kQueryFilterComponent is disabled.
  static QueryFilterData* GetInstance();

  QueryFilterData(const QueryFilterData&) = delete;
  QueryFilterData& operator=(const QueryFilterData&) = delete;
  ~QueryFilterData();

  // Called by component installer to update the version number of the
  // query filter component.
  void UpdateVersion(base::Version version);

  // Returns the current version of the query filter component.
  const std::string GetVersion() const;

  // Loads the file, parses the rules and stores them in |rules_|.
  bool PopulateDataFromComponent(std::string_view json_data);

  // Resets the |rules_| to an empty vector and the |version_| to empty.
  void ResetRulesForTesting();

  // Returns the current set of query filter rules.
  // Returns an empty vector if no rules are available.
  const std::vector<QueryFilterRule>& rules() const;

 private:
  friend class base::NoDestructor<QueryFilterData>;
  QueryFilterData();

  std::vector<QueryFilterRule> rules_;
  base::Version version_;
};

}  // namespace query_filter

#endif  // BRAVE_COMPONENTS_QUERY_FILTER_BROWSER_QUERY_FILTER_DATA_H_
