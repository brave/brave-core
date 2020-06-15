/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_browser_main_extra_parts.h"

#include "brave/browser/brave_browser_process_impl.h"
#include "brave/components/brave_shields/browser/brave_shields_p3a.h"
#include "brave/components/p2a/buildflags.h"
#include "brave/components/p2a/brave_p2a_service.h"
#include "brave/components/p3a/buildflags.h"
#include "brave/components/p3a/brave_p3a_service.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "third_party/widevine/cdm/buildflags.h"

#if !defined(OS_ANDROID)
#include "brave/browser/importer/brave_importer_p3a.h"
#include "brave/browser/p3a/p3a_core_metrics.h"
#include "chrome/browser/first_run/first_run.h"
#endif  // !defined(OS_ANDROID)

#if BUILDFLAG(BUNDLE_WIDEVINE_CDM)
#include "brave/browser/widevine/brave_widevine_bundle_manager.h"
#endif

namespace {

// Records default values for some histograms because we want these stats to be
// uploaded anyways. Corresponding components will write new values according
// to their usage scenarios.
void RecordInitialP3AValues() {
#if !defined(OS_ANDROID)
  if (first_run::IsChromeFirstRun()) {
    RecordImporterP3A(importer::ImporterType::TYPE_UNKNOWN);
  }
#endif  // !defined(OS_ANDROID)

  brave_shields::MaybeRecordShieldsUsageP3A(brave_shields::kNeverClicked,
                                            g_browser_process->local_state());
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
#if BUILDFLAG(BUNDLE_WIDEVINE_CDM)
  // Want to check as early as possible because |StartupCheck()| has some
  // fixup handling for abnormal status and run it on UI thread.
  // However, BraveBrowserProcessImpl that the owner of bundle manager is
  // created before browser thread creation.
  // So, call it after browser threads are created.
  g_brave_browser_process->brave_widevine_bundle_manager()->StartupCheck();
#endif
  // Disabled on mobile platforms, see for instance issues/6176
#if BUILDFLAG(BRAVE_P3A_ENABLED)
  // TODO(iefremov): Maybe find a better place for this initialization.
  g_brave_browser_process->brave_p3a_service()->Init(
      g_browser_process->shared_url_loader_factory());
#endif  // BUILDFLAG(BRAVE_P3A_ENABLED)

  RecordInitialP3AValues();

#if BUILDFLAG(BRAVE_P2A_ENABLED)
  // TODO(Moritz Haller): See iefremov's TODO about initialization above
  g_brave_browser_process->brave_p2a_service()->Init(
      g_browser_process->shared_url_loader_factory());
#endif  // BUILDFLAG(BRAVE_P2A_ENABLED)

  // The code below is not supported on android.
#if !defined(OS_ANDROID)
  brave::BraveWindowTracker::CreateInstance(g_browser_process->local_state());
  brave::BraveUptimeTracker::CreateInstance(g_browser_process->local_state());
#endif  // !defined(OS_ANDROID)
}
