// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/side_panel/ai_chat/ai_chat_side_panel_utils.h"

#include "base/notimplemented.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "chrome/browser/profiles/profile.h"

#if BUILDFLAG(ENABLE_AI_CHAT)
#include "brave/components/ai_chat/core/common/features.h"
#endif

namespace ai_chat {

#if !defined(TOOLKIT_VIEWS)
Browser* GetBrowserForWebContents(content::WebContents* web_contents) {
  NOTIMPLEMENTED();
  return nullptr;
}

void ClosePanel(content::WebContents* web_contents) {
  NOTIMPLEMENTED();
}
#endif

bool ShouldSidePanelBeGlobal(Profile* profile) {
#if BUILDFLAG(ENABLE_AI_CHAT)
  return profile->IsAIChatAgent() ||
         ai_chat::features::IsAIChatGlobalSidePanelEverywhereEnabled();
#else
  return false;
#endif
}

}  // namespace ai_chat
