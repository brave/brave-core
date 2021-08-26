/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/first_run/first_run_internal.h"

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/no_destructor.h"
#include "base/path_service.h"
#include "build/branding_buildflags.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/first_run/first_run.h"
#include "chrome/browser/first_run/first_run_dialog.h"
#include "chrome/browser/metrics/metrics_reporting_state.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/installer/util/google_update_settings.h"
#include "chrome/installer/util/initial_preferences.h"
#include "components/metrics/metrics_pref_names.h"
#include "components/metrics/metrics_reporting_default_state.h"
#include "components/prefs/pref_service.h"
#include "components/startup_metric_utils/browser/startup_metric_utils.h"

// All above headers copied from original first_run_internal_posix.cc are
// included to prevent below GOOGLE_CHROME_BUILD affect them.

// First run dialog is only enabled for google chrome by default.
#if defined(OFFICIAL_BUILD)
#undef BUILDFLAG_INTERNAL_GOOGLE_CHROME_BRANDING
#define BUILDFLAG_INTERNAL_GOOGLE_CHROME_BRANDING() (1)
#define FILE_LOCAL_STATE PATH_END
#endif
#include "../../../../../chrome/browser/first_run/first_run_internal_posix.cc"
#if defined(OFFICIAL_BUILD)
#undef BUILDFLAG_INTERNAL_GOOGLE_CHROME_BRANDING
#undef FILE_LOCAL_STATE
#endif
