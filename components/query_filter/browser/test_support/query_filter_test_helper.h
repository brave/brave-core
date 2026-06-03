// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_QUERY_FILTER_BROWSER_TEST_SUPPORT_QUERY_FILTER_TEST_HELPER_H_
#define BRAVE_COMPONENTS_QUERY_FILTER_BROWSER_TEST_SUPPORT_QUERY_FILTER_TEST_HELPER_H_

#include <string_view>

namespace query_filter {

namespace test {

// Populates the query filter component with default set of rules
// and clears all rules from the query filter component upon
// destruction. Must only be called with kQueryFilterComponent enabled;
// otherwise will crash the test.
class ScopedTestingQueryFilterRules {
 public:
  ScopedTestingQueryFilterRules();
  ~ScopedTestingQueryFilterRules();

  // Updates the currently stored rules with |rules_json|.
  // Returns true if the update was successful, false otherwise.
  bool UpdateRules(std::string_view rules_json);

  ScopedTestingQueryFilterRules(const ScopedTestingQueryFilterRules&) = delete;
  ScopedTestingQueryFilterRules& operator=(
      const ScopedTestingQueryFilterRules&) = delete;
};
}  // namespace test

}  // namespace query_filter

#endif  // BRAVE_COMPONENTS_QUERY_FILTER_BROWSER_TEST_SUPPORT_QUERY_FILTER_TEST_HELPER_H_
