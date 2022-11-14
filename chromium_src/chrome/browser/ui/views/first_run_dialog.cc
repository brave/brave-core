/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/first_run_dialog.h"

// We use our own FirstRun dialog on Windows/Linux.
// ShowFirstRunDialog() is re-defined at brave_first_run_dialog.cc.
#define ShowFirstRunDialog ShowFirstRunDialog_UnUsed

#include "src/chrome/browser/ui/views/first_run_dialog.cc"

#undef ShowFirstRunDialog
