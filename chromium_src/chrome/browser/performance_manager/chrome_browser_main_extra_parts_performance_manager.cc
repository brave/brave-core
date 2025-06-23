/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// This file is designed to disable certain components of the
// performance_manager to optimize CPU usage. Since Brave does not send UMA/UKM
// data, collecting these metrics is unnecessary, especially if it requires
// significant resources.
// Specifically, we:
// 1. disable PageResourceMonitor.
// 2. disable MetricsProviderDesktop::Initialize() to support overrides in
//    ChromeMetricsServiceClient.

#include "components/performance_manager/public/metrics/page_resource_monitor.h"

#if !BUILDFLAG(IS_ANDROID)
#include "chrome/browser/performance_manager/metrics/metrics_provider_desktop.h"
namespace performance_manager::metrics {
namespace {
class StubGraphOwnedDefaultImpl : public GraphOwnedDefaultImpl {};
}  // namespace
}  // namespace performance_manager::metrics

namespace performance_manager {

class FakeMetricsProviderDesktop {
 public:
  static FakeMetricsProviderDesktop* GetInstance() {
    static FakeMetricsProviderDesktop instance;
    return &instance;
  }

  void Initialize() {
    // Do nothing
  }
};
}  // namespace performance_manager

#define PageResourceMonitor StubGraphOwnedDefaultImpl
#define MetricsProviderDesktop FakeMetricsProviderDesktop
#endif  // !BUILDFLAG(IS_ANDROID)
#include <chrome/browser/performance_manager/chrome_browser_main_extra_parts_performance_manager.cc>
#if !BUILDFLAG(IS_ANDROID)
#undef MetricsProviderDesktop
#undef PageResourceMonitor
#endif  // !BUILDFLAG(IS_ANDROID)
