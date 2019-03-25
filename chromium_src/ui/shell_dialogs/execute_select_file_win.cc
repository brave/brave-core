/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ui/shell_dialogs/execute_select_file_win.h"

#include <shlobj.h>

// Disable FOS_FORCEFILESYSTEM, and replace it by
// FOS_SUPPORTSTREAMABLEITEMS.
//
// If we set FOS_FORCEFILESYSTEM and not FOS_SUPPORTSTREAMABLEITEMS,
// and you type a URL into the file picker, Windows will download that
// URL and return a pathname to the downloaded file, with network
// traffic beyond our control.
#undef FOS_FORCEFILESYSTEM
#define FOS_FORCEFILESYSTEM FOS_SUPPORTSTREAMABLEITEMS

#include "../../../../ui/shell_dialogs/execute_select_file_win.cc"  // NOLINT
