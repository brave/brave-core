/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_WINDOWS_RECALL_WINDOWS_RECALL_H_
#define BRAVE_COMPONENTS_WINDOWS_RECALL_WINDOWS_RECALL_H_

class PrefRegistrySimple;
class PrefService;

namespace windows_recall {

namespace prefs {
inline constexpr char kBlockWindowsRecall[] = "brave.block_windows_recall";
}

void RegisterLocalStatePrefs(PrefRegistrySimple* registry);

bool IsWindowsRecallAvailable();
bool IsWindowsRecallDisabled(PrefService* local_state);

}  // namespace windows_recall

#endif  // BRAVE_COMPONENTS_WINDOWS_RECALL_WINDOWS_RECALL_H_
