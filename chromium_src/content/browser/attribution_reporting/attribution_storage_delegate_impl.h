/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_ATTRIBUTION_REPORTING_ATTRIBUTION_STORAGE_DELEGATE_IMPL_H_
#define BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_ATTRIBUTION_REPORTING_ATTRIBUTION_STORAGE_DELEGATE_IMPL_H_

#include "content/browser/attribution_reporting/attribution_storage_delegate.h"

#define GetNullAggregatableReports(...)                       \
  GetNullAggregatableReports_ChromiumImpl(__VA_ARGS__) const; \
  std::vector<NullAggregatableReport> GetNullAggregatableReports(__VA_ARGS__)

#include "src/content/browser/attribution_reporting/attribution_storage_delegate_impl.h"  // IWYU pragma: export
#undef GetNullAggregatableReports

#endif  // BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_ATTRIBUTION_REPORTING_ATTRIBUTION_STORAGE_DELEGATE_IMPL_H_
