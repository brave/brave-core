// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/ai_chat_profile.h"

// Customize which profiles are considered "active" and therefore in contention
// for whether they should be the default profile on startup. If more than
// one is found then the profile picker will be shown.
// Make sure the AI Chat profile is not allowed to be the startup profile
// and does not cause the profile picker to be shown when first created.
#define BRAVE_PROFILE_PICKER_GET_STARTUP_MODE_REASON_IS_ACTIVE_PROFILE \
  if (ai_chat::IsAIChatContentAgentProfile(entry->GetPath())) {        \
    return false;                                                      \
  }

#include <chrome/browser/ui/profiles/profile_picker.cc>

#undef BRAVE_PROFILE_PICKER_GET_STARTUP_MODE_REASON_IS_ACTIVE_PROFILE
