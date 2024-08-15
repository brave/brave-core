// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/ai_chat_utils.h"

#include "brave/components/ai_chat/core/browser/utils.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/browser_context.h"

namespace ai_chat {

bool IsAllowedForContext(content::BrowserContext* context,
                         bool check_policy /*=true*/) {
  Profile* profile = Profile::FromBrowserContext(context);
  return profile->IsRegularProfile() && ai_chat::features::IsAIChatEnabled() &&
         (!check_policy || IsAIChatEnabled(profile->GetPrefs()));
}

}  // namespace ai_chat
