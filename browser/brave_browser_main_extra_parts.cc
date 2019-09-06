/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_browser_main_extra_parts.h"

#include "brave/browser/brave_browser_process_impl.h"
#include "brave/browser/importer/brave_importer_p3a.h"
#include "brave/components/brave_shields/browser/brave_shields_p3a.h"
#include "brave/components/p3a/brave_p3a_service.h"
#include "brave/components/p3a/p3a_core_metrics.h"
#include "chrome/browser/first_run/first_run.h"
#include "third_party/widevine/cdm/buildflags.h"

#if BUILDFLAG(BUNDLE_WIDEVINE_CDM)
#include "brave/browser/widevine/brave_widevine_bundle_manager.h"
#endif

BraveBrowserMainExtraParts::BraveBrowserMainExtraParts() {
}

BraveBrowserMainExtraParts::~BraveBrowserMainExtraParts() {
}

void BraveBrowserMainExtraParts::PostBrowserStart() {
  g_brave_browser_process->StartBraveServices();
}

void BraveBrowserMainExtraParts::PreMainMessageLoopRun() {
#if !defined(OS_ANDROID)
  brave::AutoImportMuon();
#endif
#if BUILDFLAG(BUNDLE_WIDEVINE_CDM)
  // Want to check as early as possible because |StartupCheck()| has some
  // fixup handling for abnormal status and run it on UI thread.
  // However, BraveBrowserProcessImpl that the owner of bundle manager is
  // created before browser thread creation.
  // So, call it after browser threads are created.
  g_brave_browser_process->brave_widevine_bundle_manager()->StartupCheck();
#endif
  // TODO(iefremov): Maybe find a better place for this initialization.
  g_brave_browser_process->brave_p3a_service()->Init();

  // Record default values for some histograms because we want these stats to be
  // uploaded anyways. Corresponding components will write new values according
  // to their usage scenarios.
  if (first_run::IsChromeFirstRun()) {
    RecordImporterP3A(importer::ImporterType::TYPE_UNKNOWN);
  }
  brave_shields::RecordShieldsUsageP3A(brave_shields::kNeverClicked,
                                       g_browser_process->local_state());

#if !defined(OS_ANDROID)
  brave::BraveWindowsTracker::CreateInstance(g_browser_process->local_state());
#endif
  brave::BraveUptimeTracker::CreateInstance(g_browser_process->local_state());
}
