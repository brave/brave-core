/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/performance_manager/metrics/memory_pressure_metrics.h"
#include "chrome/browser/performance_manager/metrics/page_resource_monitor.h"

namespace performance_manager::metrics {
class StubGraphOwnedDefaultImpl : public GraphOwnedDefaultImpl {};
}  // namespace performance_manager::metrics

#define PageResourceMonitor StubGraphOwnedDefaultImpl
#define MemoryPressureMetrics StubGraphOwnedDefaultImpl
#include "src/chrome/browser/performance_manager/chrome_browser_main_extra_parts_performance_manager.cc"
#undef MemoryPressureMetrics
#undef PageResourceMonitor
