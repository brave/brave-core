// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/ai_chat_profile.h"

#include "base/files/file_path.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile_manager.h"

namespace ai_chat {

namespace {

constexpr base::FilePath::CharType kAIChatAgentProfileDir[] =
    FILE_PATH_LITERAL("ai_chat_agent_profile");

}  // namespace

base::FilePath GetAIChatAgentProfileDir() {
  ProfileManager* profile_manager = g_browser_process->profile_manager();
  base::FilePath guest_path = profile_manager->user_data_dir();
  return guest_path.Append(kAIChatAgentProfileDir);
}

bool IsAIChatContentAgentProfile(const base::FilePath& profile_dir) {
  return profile_dir == GetAIChatAgentProfileDir();
}

bool IsAIChatContentAgentProfile(Profile* profile) {
  return profile->GetPath() == GetAIChatAgentProfileDir();
}

}  // namespace ai_chat
