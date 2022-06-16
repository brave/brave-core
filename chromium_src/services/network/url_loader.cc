/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "services/network/url_loader.h"
#include "brave/components/brave_page_graph/common/buildflags.h"

#if BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)
#include "brave/components/brave_page_graph/common/features.h"
#endif  // BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)

#define SetEnableReportingRawHeaders SetEnableReportingRawHeaders_ChromiumImpl

#include "src/services/network/url_loader.cc"

#undef SetEnableReportingRawHeaders

namespace network {

void URLLoader::SetEnableReportingRawHeaders(bool allow) {
#if BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)
  enable_reporting_raw_headers_ =
      base::FeatureList::IsEnabled(brave_page_graph::features::kPageGraph);
#else   // BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)
  SetEnableReportingRawHeaders_ChromiumImpl(allow);
#endif  // BUILDFLAG(ENABLE_BRAVE_PAGE_GRAPH)
}

}  // namespace network
