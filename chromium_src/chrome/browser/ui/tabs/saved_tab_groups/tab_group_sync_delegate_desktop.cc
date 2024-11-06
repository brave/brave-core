/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/debug/crash_logging.h"
#include "base/debug/dump_without_crashing.h"

#define BRAVE_UPDATE_LOCAL_TAB_GROUP_REPORT_AND_SUPPRESS_CRASH            \
  if (i - tab_range.start() > group.saved_tabs().size()) {                \
    SCOPED_CRASH_KEY_NUMBER("TgsDiag", "i_minus_tab_range_start",         \
                            static_cast<int>(i - tab_range.start()));     \
    SCOPED_CRASH_KEY_NUMBER("TgsDiag", "tab_range_length",                \
                            static_cast<int>(tab_range.length()));        \
    SCOPED_CRASH_KEY_NUMBER("TgsDiag", "group_saved_tabs_size",           \
                            static_cast<int>(group.saved_tabs().size())); \
    base::debug::DumpWithoutCrashing();                                   \
    /* With the continue below the crash happens anyway later at*/        \
    /* SavedTabGroupModelListener::ConnectToLocalTabGroup,*/              \
    /* CHECK_EQ(local_group_size, tab_guid_mapping.size()). At least we*/ \
    /* will have additional data assigned to crash*/                      \
    continue;                                                             \
  }

#include "src/chrome/browser/ui/tabs/saved_tab_groups/tab_group_sync_delegate_desktop.cc"

#undef BRAVE_UPDATE_LOCAL_TAB_GROUP_REPORT_AND_SUPPRESS_CRASH
