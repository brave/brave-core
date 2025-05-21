/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/misc_metrics/extension_metrics.h"

#include "base/containers/fixed_flat_set.h"
#include "base/metrics/histogram_macros.h"
#include "brave/browser/extensions/manifest_v2/brave_extensions_manifest_v2_installer.h"
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
constexpr auto kManifestV2ExtensionIDExceptions =
    base::MakeFixedFlatSet<std::string_view>(
        base::sorted_unique,
        {
            // PDF Viewer
            "mhjfbmdgcfjbbpaeojofohoefgiehjai",
            // Brave
            "mnojpmjdmbbfmejpflffifhffcmidifd",
        });
constexpr base::TimeDelta kReportDebounceTime = base::Seconds(10);

}  // namespace

ExtensionMetrics::ExtensionMetrics(
    extensions::ExtensionRegistry* extension_registry)
    : extension_registry_(extension_registry), observation_(this) {
  CHECK(extension_registry);
  for (const auto& extension : extension_registry_->enabled_extensions()) {
    OnExtensionLoaded(extension_registry_->browser_context(), extension.get());
  }
  if (extension_registry) {
    observation_.Observe(extension_registry);
  }
  ScheduleMetricsReport();
}

ExtensionMetrics::~ExtensionMetrics() = default;

void ExtensionMetrics::Shutdown() {
  report_debounce_timer_.Stop();
  if (extension_registry_) {
    observation_.Reset();
    extension_registry_ = nullptr;
  }
}

void ExtensionMetrics::OnExtensionLoaded(
    content::BrowserContext* browser_context,
    const extensions::Extension* extension) {
  if (kPopularAdBlockExtensions.contains(extension->id())) {
    adblock_extensions_loaded_.insert(extension->id());
  }

  // Check if this is a Manifest V2 extension
  if (extension->manifest_version() == 2 &&
      !kManifestV2ExtensionIDExceptions.contains(extension->id())) {
    manifest_v2_extensions_loaded_.insert(extension->id());
  }

  // Check if this is a pre-configured Manifest V2 extension
  if (extensions_mv2::kPreconfiguredManifestV2Extensions.contains(
          extension->id())) {
    select_manifest_v2_extensions_loaded_.insert(extension->id());
  }

  ScheduleMetricsReport();
}

void ExtensionMetrics::OnExtensionUninstalled(
    content::BrowserContext* browser_context,
    const extensions::Extension* extension,
    extensions::UninstallReason reason) {
  if (kPopularAdBlockExtensions.contains(extension->id())) {
    adblock_extensions_loaded_.erase(extension->id());
  }

  if (extension->manifest_version() == 2 &&
      !kManifestV2ExtensionIDExceptions.contains(extension->id())) {
    manifest_v2_extensions_loaded_.erase(extension->id());
  }

  // Check if this is a pre-configured Manifest V2 extension
  if (extensions_mv2::kPreconfiguredManifestV2Extensions.contains(
          extension->id())) {
    select_manifest_v2_extensions_loaded_.erase(extension->id());
  }

  ScheduleMetricsReport();
}

void ExtensionMetrics::ScheduleMetricsReport() {
  report_debounce_timer_.Start(
      FROM_HERE, kReportDebounceTime,
      base::BindOnce(&ExtensionMetrics::ReportMetrics, base::Unretained(this)));
}

void ExtensionMetrics::ReportMetrics() {
  UMA_HISTOGRAM_BOOLEAN(kAdblockExtensionsHistogramName,
                        !adblock_extensions_loaded_.empty());
  UMA_HISTOGRAM_BOOLEAN(kManifestV2ExtensionsHistogramName,
                        !manifest_v2_extensions_loaded_.empty());
  UMA_HISTOGRAM_BOOLEAN(kSelectManifestV2ExtensionsHistogramName,
                        !select_manifest_v2_extensions_loaded_.empty());
}

}  // namespace misc_metrics
