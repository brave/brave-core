/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/performance_manager/metrics/page_resource_monitor.h"

namespace performance_manager::metrics {
namespace {
class StubGraphOwnedDefaultImpl : public GraphOwnedDefaultImpl {};
}  // namespace
}  // namespace performance_manager::metrics

// PageResourceMonitor collects data only to send UMA/UKM. It means it does
// nothing in Brave, but the collection is CPU-intensive. Disable it to save
// CPU during startup.
#define PageResourceMonitor StubGraphOwnedDefaultImpl
#include "src/chrome/browser/performance_manager/chrome_browser_main_extra_parts_performance_manager.cc"
#undef PageResourceMonitor
