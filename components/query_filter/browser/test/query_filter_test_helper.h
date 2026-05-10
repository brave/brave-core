// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_QUERY_FILTER_BROWSER_TEST_QUERY_FILTER_TEST_HELPER_H_
#define BRAVE_COMPONENTS_QUERY_FILTER_BROWSER_TEST_QUERY_FILTER_TEST_HELPER_H_

namespace query_filter {

namespace test {
// Populates the query filter component with default set of rules.
// |kQueryFilterComponent| flag must be enabled before calling this method or
// the assertion will fail.
void SetupWithDefaultQueryFilterRules();

// Clears all rules from the query filter component.
// |kQueryFilterComponent| flag must be enabled before calling this method or
// the assertion will fail.
void RemoveDefaultRules();
}  // namespace test

}  // namespace query_filter

#endif  // BRAVE_COMPONENTS_QUERY_FILTER_BROWSER_TEST_QUERY_FILTER_TEST_HELPER_H_
