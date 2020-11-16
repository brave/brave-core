/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/profile_resetter/reset_report_uploader.h"

#define DispatchReportInternal DispatchReportInternal_ChromiumImpl
#include "../../../../../chrome/browser/profile_resetter/reset_report_uploader.cc"
#undef DispatchReportInternal

void ResetReportUploader::DispatchReportInternal(
    const std::string& request_data) {
  return;
}
