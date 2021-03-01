/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_browser_main_extra_parts.h"

#include "base/metrics/histogram_macros.h"
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/components/brave_shields/browser/brave_shields_p3a.h"
#include "brave/components/p3a/brave_p3a_service.h"
#include "brave/components/p3a/buildflags.h"
#include "components/metrics/metrics_pref_names.h"
#include "components/prefs/pref_service.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

#if !defined(OS_ANDROID)
#include "brave/browser/importer/brave_importer_p3a.h"
#include "brave/browser/p3a/p3a_core_metrics.h"
#include "brave/browser/ui/webui/new_tab_page/brave_new_tab_message_handler.h"
#include "chrome/browser/first_run/first_run.h"
#endif  // !defined(OS_ANDROID)

namespace {

// Records default values for some histograms because we want these stats to be
// uploaded anyway. Corresponding components will write new values according
// to their usage scenarios.
//
// For profile specific values, see browser/profiles/profile_util.cc
void RecordInitialP3AValues() {
#if !defined(OS_ANDROID)
  if (first_run::IsChromeFirstRun()) {
    RecordImporterP3A(importer::ImporterType::TYPE_UNKNOWN);
  }

  BraveNewTabMessageHandler::RecordInitialP3AValues(
      g_browser_process->local_state());
#endif  // !defined(OS_ANDROID)

  brave_shields::MaybeRecordShieldsUsageP3A(brave_shields::kNeverClicked,
                                            g_browser_process->local_state());

  // Record crash reporting status stats.
  const bool crash_reports_enabled = g_browser_process->local_state()->
      GetBoolean(metrics::prefs::kMetricsReportingEnabled);
  UMA_HISTOGRAM_BOOLEAN("Brave.Core.CrashReportsEnabled",
                        crash_reports_enabled);
}

}  // namespace

BraveBrowserMainExtraParts::BraveBrowserMainExtraParts() {
}

BraveBrowserMainExtraParts::~BraveBrowserMainExtraParts() {
}

void BraveBrowserMainExtraParts::PostBrowserStart() {
  g_brave_browser_process->StartBraveServices();
}

void BraveBrowserMainExtraParts::PreMainMessageLoopRun() {
  // Disabled on mobile platforms, see for instance issues/6176
#if BUILDFLAG(BRAVE_P3A_ENABLED)
  // TODO(iefremov): Maybe find a better place for this initialization.
  g_brave_browser_process->brave_p3a_service()->Init(
      g_browser_process->shared_url_loader_factory());
#endif  // BUILDFLAG(BRAVE_P3A_ENABLED)

  RecordInitialP3AValues();

  // The code below is not supported on android.
#if !defined(OS_ANDROID)
  brave::BraveWindowTracker::CreateInstance(g_browser_process->local_state());
  brave::BraveUptimeTracker::CreateInstance(g_browser_process->local_state());
#endif  // !defined(OS_ANDROID)
}
