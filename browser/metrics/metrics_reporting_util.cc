/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/metrics/metrics_reporting_util.h"

#include "base/logging.h"
#include "base/notreached.h"
#include "base/types/cxx23_to_underlying.h"
#include "brave/browser/metrics/brave_metrics_service_accessor.h"
#include "brave/browser/metrics/buildflags/buildflags.h"
#include "brave/components/constants/pref_names.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/metrics/metrics_reporting_state.h"
#include "chrome/common/channel_info.h"
#include "components/prefs/pref_service.h"
#include "components/version_info/channel.h"

bool GetDefaultPrefValueForMetricsReporting() {
  auto channel = chrome::GetChannel();
  switch (channel) {
    case version_info::Channel::STABLE:
      return false;
    case version_info::Channel::BETA:    // fall through
    case version_info::Channel::DEV:     // fall through
    case version_info::Channel::CANARY:
      return true;
    case version_info::Channel::UNKNOWN:
      return false;
  }
  NOTREACHED() << "Unexpected value for channel: "
               << base::to_underlying(channel);
}

bool ShouldShowCrashReportPermissionAskDialog() {
#if BUILDFLAG(ENABLE_CRASH_DIALOG)
  PrefService* local_prefs = g_browser_process->local_state();
  if (local_prefs->GetBoolean(kDontAskForCrashReporting))
    return false;

  if (IsMetricsReportingPolicyManaged())
    return false;

  if (BraveMetricsServiceAccessor::IsMetricsAndCrashReportingEnabled())
    return false;

  return true;
#else
  return false;
#endif
}
