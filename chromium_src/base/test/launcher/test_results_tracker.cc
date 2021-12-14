/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// Upstream deliberately avoids including the failure's details and stack trace
// in the result XML for the xUnit format, so we need to add it explicitly to be
// able to visualize the data directly from Jenkins for each failed test.
#define TEST_RESULTS_TRACKER_ADD_FAILURE_DETAILS                             \
  std::string failure_data;                                                  \
  for (const TestResultPart& result_part : result.test_result_parts) {       \
    failure_data += StringPrintf(                                            \
        "Failure at %s (line %d):\n\n%s\n\n", result_part.file_name.c_str(), \
        result_part.line_number, result_part.message.c_str());               \
  }                                                                          \
  fprintf(out_,                                                              \
          "      <failure message=\"[  FAILED  ] %s.%s\" "                   \
          "type=\"\"><![CDATA[%s]]></failure>\n",                            \
          testsuite_name.c_str(), result.GetTestName().c_str(),              \
          failure_data.c_str());

#include "src/base/test/launcher/test_results_tracker.cc"

#undef TEST_RESULTS_TRACKER_ADD_FAILURE_DETAILS
