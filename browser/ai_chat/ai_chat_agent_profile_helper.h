// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_AI_CHAT_AI_CHAT_AGENT_PROFILE_HELPER_H_
#define BRAVE_BROWSER_AI_CHAT_AI_CHAT_AGENT_PROFILE_HELPER_H_

#include "base/functional/callback.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"

static_assert(BUILDFLAG(ENABLE_BRAVE_AI_CHAT_AGENT_PROFILE));

class Browser;
class Profile;

namespace ai_chat {

// Creates or loads a profile for the purposes of experimental AI content agent
// features. This "AI Chat Agent Profile" will have all the features of a
// regular profile: persistance and history but remain isolated from any regular
// user profiles. It provides a space for the user to collaborate with the AI on
// browsing activities. Whilst the user can open the profile, configure it via
// the AI Chat UI, and perform navigations in the profile themselves, efforts
// are made to ensure the profile does not become the default profile, e.g. not
// showing the profile picker dialog at browser startup just because we have
// created this profile.

// TODO(https://github.com/brave/brave-browser/issues/48190): Move to
// AIChatAgentProfileManager.

void OpenBrowserWindowForAIChatAgentProfile(Profile& from_profile);

#if !BUILDFLAG(IS_ANDROID)
void OpenBrowserWindowForAIChatAgentProfileForTesting(
    Profile& from_profile,
    base::OnceCallback<void(Browser*)> callback);
#endif

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_AI_CHAT_AI_CHAT_AGENT_PROFILE_HELPER_H_
