// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_SESSIONS_CORE_COMMAND_STORAGE_MANAGER_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_SESSIONS_CORE_COMMAND_STORAGE_MANAGER_H_

#define CommandStorageManagerTestHelper                              \
  CommandStorageManagerTestHelper;                                   \
  FRIEND_TEST_ALL_PREFIXES(BraveTabStripModelRenamingTabBrowserTest, \
                           SettingCustomTabTitle_Session)

#include <components/sessions/core/command_storage_manager.h>  // IWYU pragma: export

#undef CommandStorageManagerTestHelper

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_SESSIONS_CORE_COMMAND_STORAGE_MANAGER_H_
