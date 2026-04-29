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
#include "brave/components/query_filter/common/schema.h"

namespace query_filter {

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

  // Parses |json_data| and stores the rules in |rules_|.
  // |json_data| must follow the schema in query_filter_schema.idl.
  // Parsing is relaxed: any rule entry that does not match schema::Rule is
  // ignored.
  // Returns true if the root array was parsed successfully and at least one
  // valid rule was found; false otherwise.
  bool PopulateDataFromComponent(std::string_view json_data);

  // Returns the current set of query filter rules.
  // Returns an empty ruleset if no rules are available.
  const std::vector<schema::Rule>& rules() const { return rules_; }

  // A test only method to reset the |rules_| and |version_|.
  void ResetRulesForTesting() noexcept;

 private:
  friend class base::NoDestructor<QueryFilterData>;
  QueryFilterData();

  // The container to hold the query filter rules.
  std::vector<schema::Rule> rules_;
  // Holds the current version of the query filter component.
  // This gets populated by the QueryFilterComponentInstallerPolicy when the
  // query filter component is ready.
  base::Version version_;
};

}  // namespace query_filter

#endif  // BRAVE_COMPONENTS_QUERY_FILTER_BROWSER_QUERY_FILTER_DATA_H_
