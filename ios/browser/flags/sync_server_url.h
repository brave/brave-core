// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_FLAGS_SYNC_SERVER_URL_H_
#define BRAVE_IOS_BROWSER_FLAGS_SYNC_SERVER_URL_H_

namespace base {
class CommandLine;
class ListValue;
}  // namespace base

namespace flags_ui {
class FlagsStorage;
}

namespace brave_sync {

inline constexpr char kSyncServerUrlFlagName[] =
    "brave-override-sync-server-url";

// Handles the string value entered in brave://flags.
void SetSyncServerUrlFlag(flags_ui::FlagsStorage* flags_storage,
                          const base::ListValue& args);

// Run after the startup crash keys are recorded and before profiles are loaded.
void ApplySyncServerUrlFromFlags(const flags_ui::FlagsStorage& flags_storage,
                                 base::CommandLine& command_line);

}  // namespace brave_sync

#endif  // BRAVE_IOS_BROWSER_FLAGS_SYNC_SERVER_URL_H_
