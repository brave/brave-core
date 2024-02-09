/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_MISC_METRICS_EXTENSION_METRICS_H_
#define BRAVE_BROWSER_MISC_METRICS_EXTENSION_METRICS_H_

#include <string>

#include "base/containers/flat_set.h"
#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "base/timer/timer.h"
#include "components/keyed_service/core/keyed_service.h"
#include "extensions/browser/extension_registry_observer.h"
#include "extensions/browser/uninstall_reason.h"

namespace content {
class BrowserContext;
}

namespace extensions {
class Extension;
class ExtensionRegistry;
}  // namespace extensions

namespace misc_metrics {

inline constexpr char kAdblockExtensionsHistogramName[] =
    "Brave.Extensions.AdBlock";

// Monitors installation/uninstallation of third-party extensions
// and reports relevant metrics via P3A.
class ExtensionMetrics : public extensions::ExtensionRegistryObserver {
 public:
  explicit ExtensionMetrics(
      extensions::ExtensionRegistry* extension_registry);
  ~ExtensionMetrics() override;

  ExtensionMetrics(const ExtensionMetrics&) = delete;
  ExtensionMetrics& operator=(const ExtensionMetrics&) = delete;

  void Shutdown();

 private:
  // extensions::ExtensionRegistryObserver:
  void OnExtensionLoaded(content::BrowserContext* browser_context,
                         const extensions::Extension* extension) override;
  void OnExtensionUninstalled(content::BrowserContext* browser_context,
                              const extensions::Extension* extension,
                              extensions::UninstallReason reason) override;

  void ScheduleAdBlockMetricReport();
  void ReportAdBlockMetric();

  base::flat_set<std::string> adblock_extensions_loaded_;
  raw_ptr<extensions::ExtensionRegistry> extension_registry_;
  base::ScopedObservation<extensions::ExtensionRegistry,
                          ExtensionMetrics>
      observation_;

  base::OneShotTimer report_debounce_timer_;
};

}  // namespace misc_metrics

#endif  // BRAVE_BROWSER_MISC_METRICS_EXTENSION_METRICS_H_
