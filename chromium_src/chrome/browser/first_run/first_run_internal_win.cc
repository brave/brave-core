/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/first_run/first_run_dialog.h"
#include "chrome/browser/first_run/first_run_internal.h"

namespace {

bool ShouldShowFirstRunDialog() {
#if defined(OFFICIAL_BUILD)
  return first_run::internal::IsOrganicFirstRun();
#else
  return false;
#endif
}

}  // namespace

#define DoPostImportPlatformSpecificTasks \
  DoPostImportPlatformSpecificTasks_ChromiumImpl

#include "src/chrome/browser/first_run/first_run_internal_win.cc"

#undef DoPostImportPlatformSpecificTasks

namespace first_run {
namespace internal {

void DoPostImportPlatformSpecificTasks(Profile* profile) {
  if (ShouldShowFirstRunDialog()) {
    ShowFirstRunDialog(profile);
  }

  DoPostImportPlatformSpecificTasks_ChromiumImpl(profile);
}

}  // namespace internal
}  // namespace first_run
