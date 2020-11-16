/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_PROFILE_RESETTER_RESET_REPORT_UPLOADER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_PROFILE_RESETTER_RESET_REPORT_UPLOADER_H_

#include <string>

#define DispatchReportInternal                                          \
  DispatchReportInternal_ChromiumImpl(const std::string& request_data); \
  void DispatchReportInternal

#include "../../../../../chrome/browser/profile_resetter/reset_report_uploader.h"
#undef DispatchReportInternal

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_PROFILE_RESETTER_RESET_REPORT_UPLOADER_H_
