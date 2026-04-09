// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_QUERY_FILTER_BROWSER_QUERY_FILTER_SERVICE_H_
#define BRAVE_COMPONENTS_QUERY_FILTER_BROWSER_QUERY_FILTER_SERVICE_H_

#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/memory/singleton.h"
#include "base/memory/weak_ptr.h"

namespace query_filter {

// One rule from query-filter.json: URL patterns and query params to strip.
struct QueryFilterRule {
  std::vector<std::string> include;
  std::vector<std::string> exclude;
  std::vector<std::string> params;
};

// Loads and holds rules from the query filter component (query-filter.json).
// See https://github.com/brave/adblock-lists/blob/master/brave-lists/query-filter.json
class QueryFilterService {
 public:
  QueryFilterService(const QueryFilterService&) = delete;
  QueryFilterService& operator=(const QueryFilterService&) = delete;
  ~QueryFilterService();

  static QueryFilterService* GetInstance();

  void OnComponentReady(const base::FilePath& install_dir);

  bool is_ready() const { return is_ready_; }
  const std::vector<QueryFilterRule>& rules() const { return rules_; }

  void SetRulesJsonForTesting(const std::string& json);

 private:
  friend struct base::DefaultSingletonTraits<QueryFilterService>;

  QueryFilterService();

  void OnRulesJsonLoaded(const std::string& contents);
  void ParseRulesJson(const std::string& contents);

  base::FilePath install_dir_;
  std::vector<QueryFilterRule> rules_;
  bool is_ready_ = false;
  base::WeakPtrFactory<QueryFilterService> weak_factory_{this};
};

}  // namespace query_filter

#endif  // BRAVE_COMPONENTS_QUERY_FILTER_BROWSER_QUERY_FILTER_SERVICE_H_
