/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/first_run/first_run.h"

#include "brave/browser/first_run/first_run.h"

#define IsMetricsReportingOptIn IsMetricsReportingOptIn_ChromiumImpl
#include "src/chrome/browser/first_run/first_run.cc"
#undef IsMetricsReportingOptIn

namespace first_run {

bool IsMetricsReportingOptIn() {
  return brave::first_run::IsMetricsReportingOptIn();
}

}  // namespace first_run
