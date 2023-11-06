/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_browser_main_extra_parts.h"

#include "base/metrics/histogram_macros.h"
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/components/brave_shields/content/browser/brave_shields_p3a.h"
#include "brave/components/p3a/buildflags.h"
#include "brave/components/p3a/p3a_service.h"
#include "components/metrics/metrics_pref_names.h"
#include "components/prefs/pref_service.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

#if !BUILDFLAG(IS_ANDROID)
#include "brave/browser/importer/brave_importer_p3a.h"
#include "brave/browser/p3a/p3a_core_metrics.h"
#include "brave/browser/ui/webui/new_tab_page/brave_new_tab_message_handler.h"
#include "chrome/browser/first_run/first_run.h"
#endif  // !BUILDFLAG(IS_ANDROID)

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "extensions/common/extension.h"
#endif  // BUILDFLAG(ENABLE_EXTENSIONS)

namespace {

// Records default values for some histograms because we want these stats to be
// uploaded anyways. Corresponding components will write new values according
// to their usage scenarios.
//
// For profile specific values, see browser/profiles/profile_util.cc
void RecordInitialP3AValues() {
#if !BUILDFLAG(IS_ANDROID)
  if (first_run::IsChromeFirstRun()) {
    RecordImporterP3A(importer::ImporterType::TYPE_UNKNOWN);
  }

  BraveNewTabMessageHandler::RecordInitialP3AValues(
      g_browser_process->local_state());
#endif  // !BUILDFLAG(IS_ANDROID)

  brave_shields::MaybeRecordShieldsUsageP3A(brave_shields::kNeverClicked,
                                            g_browser_process->local_state());

  // Record crash reporting status stats.
  const bool crash_reports_enabled = g_browser_process->local_state()->
      GetBoolean(metrics::prefs::kMetricsReportingEnabled);
  UMA_HISTOGRAM_BOOLEAN("Brave.Core.CrashReportsEnabled",
                        crash_reports_enabled);
}

}  // namespace

BraveBrowserMainExtraParts::BraveBrowserMainExtraParts() = default;

BraveBrowserMainExtraParts::~BraveBrowserMainExtraParts() = default;

void BraveBrowserMainExtraParts::PreProfileInit() {
#if BUILDFLAG(ENABLE_EXTENSIONS)
  // Disable warnings related to Manifest V2 deprecation
  extensions::Extension::
      set_silence_deprecated_manifest_version_warnings_for_testing(true);
#endif  // BUILDFLAG(ENABLE_EXTENSIONS)
#if BUILDFLAG(IS_WIN)
  immersive_context_ = std::make_unique<ImmersiveContextWin>();
#endif
}

void BraveBrowserMainExtraParts::PostBrowserStart() {
  g_brave_browser_process->StartBraveServices();
}

void BraveBrowserMainExtraParts::PreMainMessageLoopRun() {
  // Disabled on mobile platforms, see for instance issues/6176
  if (g_brave_browser_process->p3a_service() != nullptr) {
    // TODO(iefremov): Maybe find a better place for this initialization.
    g_brave_browser_process->p3a_service()->Init(
        g_browser_process->shared_url_loader_factory());
  }

  RecordInitialP3AValues();

  // The code below is not supported on android.
#if !BUILDFLAG(IS_ANDROID)
  brave::BraveWindowTracker::CreateInstance(g_browser_process->local_state());
#endif  // !BUILDFLAG(IS_ANDROID)
}
