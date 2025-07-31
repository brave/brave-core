// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_AI_CHAT_AI_CHAT_PROFILE_H_
#define BRAVE_BROWSER_AI_CHAT_AI_CHAT_PROFILE_H_

#include "base/files/file_path.h"

class Profile;

namespace ai_chat {

base::FilePath GetAIChatAgentProfileDir();

// Returns true if the full profile path is the semi-built-in AI Chat Agent
// profile, i.e. the result of Profile::GetPath() is the same as
// GetAIChatAgentProfileDir().
bool IsAIChatContentAgentProfile(const base::FilePath& profile_dir);

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_AI_CHAT_AI_CHAT_PROFILE_H_
