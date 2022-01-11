/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// Upstream deliberately avoids including the failure's details and stack trace
// in the result XML for the xUnit format, so we need to add it explicitly to be
// able to visualize the data directly from Jenkins for each failed test.
#define TEST_RESULTS_TRACKER_ADD_FAILURE_DETAILS                \
  fprintf(out_,                                                 \
          "      <failure message=\"[  FAILED  ] %s.%s\" "      \
          "type=\"\"><![CDATA[%s]]></failure>\n",               \
          testsuite_name.c_str(), result.GetTestName().c_str(), \
          result.output_snippet.c_str());

#include "src/base/test/launcher/test_results_tracker.cc"

#undef TEST_RESULTS_TRACKER_ADD_FAILURE_DETAILS
