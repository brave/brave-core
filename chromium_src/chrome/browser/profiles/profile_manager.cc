/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/profiles/brave_profile_manager.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_AI_CHAT)
#include "brave/components/ai_chat/core/common/features.h"
#endif

// static
std::vector<Profile*> ProfileManager::GetLastOpenedProfiles() {
  // Don't include AI Chat agent profile in the list, to avoid
  // re-opening it on startup and having users mistake it for their
  // main profile, adding authentication they might not want exposed
  // to the agent.
  // Alternatives considered:
  // - Intercepting SaveActiveProfiles. Problematic because we would either have
  // to first remove the profile from `active_profiles_` (which
  // `OnBrowserClosed` expects the profile to be in the list), or perform a
  // quick subsequent pref update (which could cause side effects).
  std::vector<Profile*> profiles = GetLastOpenedProfiles_ChromiumImpl();
#if BUILDFLAG(ENABLE_BRAVE_AI_CHAT_AGENT_PROFILE)
  if (ai_chat::features::IsAIChatAgentProfileEnabled()) {
    std::erase_if(profiles,
                  [](Profile* profile) { return profile->IsAIChatAgent(); });
  }
#endif
  return profiles;
}

#define GetLastOpenedProfiles GetLastOpenedProfiles_ChromiumImpl
#include <chrome/browser/profiles/profile_manager.cc>
#undef GetLastOpenedProfiles
