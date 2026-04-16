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

struct QueryFilterRule {
  QueryFilterRule();
  ~QueryFilterRule();

  QueryFilterRule(const QueryFilterRule&);
  QueryFilterRule& operator=(const QueryFilterRule&);

  std::vector<std::string> include;
  std::vector<std::string> exclude;
  std::vector<std::string> params;
};

// Singleton which loads and holds rules from the query filter component
// (query-filter.json).
// See
// https://github.com/brave/adblock-lists/blob/master/brave-lists/query-filter.json
class QueryFilterService {
 public:
  QueryFilterService(const QueryFilterService&) = delete;
  QueryFilterService& operator=(const QueryFilterService&) = delete;
  ~QueryFilterService();

  static QueryFilterService* GetInstance();

  // Loads the file, parses the rules and stores them in |rules_|.
  void OnComponentReady(const base::FilePath& install_dir);

  const std::vector<QueryFilterRule>& rules() const { return rules_; }

 private:
  friend class QueryFilterServiceTest;
  friend struct base::DefaultSingletonTraits<QueryFilterService>;

  QueryFilterService();

  // Called when the rules JSON is loaded from the DAT file.
  void OnRulesJsonLoaded(const std::string& contents);
  // Resets first the current rules and then parses the new ones from the given
  // JSON string.
  void ParseRulesJson(const std::string_view contents);

  base::FilePath install_dir_;
  std::vector<QueryFilterRule> rules_;
  base::WeakPtrFactory<QueryFilterService> weak_factory_{this};
};

}  // namespace query_filter

#endif  // BRAVE_COMPONENTS_QUERY_FILTER_BROWSER_QUERY_FILTER_SERVICE_H_
