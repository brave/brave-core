// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_AI_CHAT_AI_CHAT_AGENT_PROFILE_MANAGER_H_
#define BRAVE_BROWSER_AI_CHAT_AI_CHAT_AGENT_PROFILE_MANAGER_H_

#include "base/files/file_path.h"
#include "chrome/browser/profiles/profile_attributes_storage.h"
#include "chrome/browser/profiles/profile_manager_observer.h"

class ProfileManager;

namespace ai_chat {

// Observes the profile system and manages state for any
// created AI Chat agent profiles.
// Also observes any browser created and ensures the AI Chat side
// panel is initially opened.
class AIChatAgentProfileManager : public ProfileAttributesStorage::Observer,
                                  public ProfileManagerObserver {
 public:
  explicit AIChatAgentProfileManager(ProfileManager* profile_manager);
  ~AIChatAgentProfileManager() override;

  // ProfileAttributesStorage::Observer
  void OnProfileAdded(const base::FilePath& profile_path) override;

  // ProfileManagerObserver
  void OnProfileAdded(Profile* profile) override;

 private:
  bool is_added_profile_new_profile_ = false;

  raw_ptr<ProfileManager> profile_manager_ = nullptr;
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_AI_CHAT_AI_CHAT_AGENT_PROFILE_MANAGER_H_
