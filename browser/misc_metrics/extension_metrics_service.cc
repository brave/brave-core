/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/misc_metrics/extension_metrics_service.h"

#include "base/containers/fixed_flat_set.h"
#include "base/metrics/histogram_macros.h"
#include "extensions/browser/extension_registry.h"

namespace misc_metrics {

namespace {

constexpr auto kPopularAdBlockExtensions =
    base::MakeFixedFlatSet<std::string_view>(
        base::sorted_unique,
        {
            // AdGuard
            "bgnkhhnnamicmpeenaelnjfhikgbkllg",
            // uBO
            "cjpalhdlnbpafiamejdnhcphjbkeiagm",
            // Ghostery
            "mlomiejdfkolichcflejclcbmpeaniij",
            // AdBlocker Ultimate
            "ohahllgiabjaoigichmmfljhkcfikeof",
        });
constexpr base::TimeDelta kReportDebounceTime = base::Seconds(10);

}  // namespace

ExtensionMetricsService::ExtensionMetricsService(
    extensions::ExtensionRegistry* extension_registry)
    : extension_registry_(extension_registry), observation_(this) {
  CHECK(extension_registry);
  for (const auto& extension : extension_registry_->enabled_extensions()) {
    OnExtensionLoaded(extension_registry_->browser_context(), extension.get());
  }
  if (extension_registry) {
    observation_.Observe(extension_registry);
  }
  ScheduleAdBlockMetricReport();
}

ExtensionMetricsService::~ExtensionMetricsService() = default;

void ExtensionMetricsService::Shutdown() {
  report_debounce_timer_.Stop();
  if (extension_registry_) {
    observation_.Reset();
    extension_registry_ = nullptr;
  }
}

void ExtensionMetricsService::OnExtensionLoaded(
    content::BrowserContext* browser_context,
    const extensions::Extension* extension) {
  if (kPopularAdBlockExtensions.contains(extension->id())) {
    adblock_extensions_loaded_.insert(extension->id());
    ScheduleAdBlockMetricReport();
  }
}

void ExtensionMetricsService::OnExtensionUninstalled(
    content::BrowserContext* browser_context,
    const extensions::Extension* extension,
    extensions::UninstallReason reason) {
  if (kPopularAdBlockExtensions.contains(extension->id())) {
    adblock_extensions_loaded_.erase(extension->id());
    ScheduleAdBlockMetricReport();
  }
}

void ExtensionMetricsService::ScheduleAdBlockMetricReport() {
  report_debounce_timer_.Start(
      FROM_HERE, kReportDebounceTime,
      base::BindOnce(&ExtensionMetricsService::ReportAdBlockMetric,
                     base::Unretained(this)));
}

void ExtensionMetricsService::ReportAdBlockMetric() {
  UMA_HISTOGRAM_BOOLEAN(kAdblockExtensionsHistogramName,
                        !adblock_extensions_loaded_.empty());
}

}  // namespace misc_metrics
