// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/ai_chat_agent_profile_manager.h"

#include "brave/components/ai_chat/core/browser/utils.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/constants/brave_constants.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "third_party/skia/include/core/SkColor.h"

#if !BUILDFLAG(IS_ANDROID)
#include "chrome/browser/themes/theme_service.h"
#include "chrome/browser/themes/theme_service_factory.h"
#endif

static_assert(BUILDFLAG(ENABLE_BRAVE_AI_CHAT_AGENT_PROFILE));

namespace ai_chat {

namespace {

#if !BUILDFLAG(IS_ANDROID)
const SkColor kAIChatAgentProfileThemeColor = SkColorSetRGB(253, 58, 122);
#endif

const char16_t kAIChatAgentProfileName[] = u"AI browsing";

}  // namespace

AIChatAgentProfileManager::AIChatAgentProfileManager(
    ProfileManager* profile_manager)
    : profile_manager_(profile_manager) {
  CHECK(profile_manager_);
  CHECK(ai_chat::features::IsAIChatAgentProfileEnabled());
  profile_manager_->GetProfileAttributesStorage().AddObserver(this);
  profile_manager_->AddObserver(this);
}

AIChatAgentProfileManager::~AIChatAgentProfileManager() {
  profile_manager_->RemoveObserver(this);
  profile_manager_->GetProfileAttributesStorage().RemoveObserver(this);
}

void AIChatAgentProfileManager::OnProfileAdded(
    const base::FilePath& profile_path) {
  if (profile_path.BaseName().value() == brave::kAIChatAgentProfileDir) {
    // Some actions we might want to do only if the profile is new, but we
    // can't access the Profile here yet. Wait for ProfileManager to call
    // OnProfileAdded(Profile*) and remember it's new.
    is_added_profile_new_ai_chat_agent_profile_ = true;
  }
}

void AIChatAgentProfileManager::OnProfileAdded(Profile* profile) {
  if (is_added_profile_new_ai_chat_agent_profile_ && profile->IsAIChatAgent()) {
    is_added_profile_new_ai_chat_agent_profile_ = false;
    // Assume user has opted-in in some profile already in order to get
    // here, so we can copy that preference.
    ai_chat::SetUserOptedIn(profile->GetPrefs(), true);

    // Set profile name so that the user can identify the profile
    // in the various profile list UIs.
    // TODO(https://github.com/brave/brave-browser/issues/48164): set an avatar
    profile_manager_->GetProfileAttributesStorage()
        .GetProfileAttributesWithPath(profile->GetPath())
        ->SetLocalProfileName(kAIChatAgentProfileName, false);

#if !BUILDFLAG(IS_ANDROID)
    // Set theme
    auto* theme_service = ThemeServiceFactory::GetForProfile(profile);
    theme_service->SetUserColor(kAIChatAgentProfileThemeColor);
#endif
  }
}

}  // namespace ai_chat
