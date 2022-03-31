/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define IsMetricsReportingOptIn IsMetricsReportingOptIn_UnUsed
#include "src/chrome/browser/first_run/first_run.cc"
#undef IsMetricsReportingOptIn

namespace first_run {

// This is only used for determine whether crash report checkbox in the first
// run dialog is checked or not by default. See the comments of this upstream
// function. Returning true means crash report is unchecked by default.
bool IsMetricsReportingOptIn() {
  return true;
}

}  // namespace first_run
