// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_AI_CHAT_AI_CHAT_BROWSERTEST_UTIL_H_
#define BRAVE_BROWSER_AI_CHAT_AI_CHAT_BROWSERTEST_UTIL_H_

#include "brave/browser/ai_chat/model_service_factory.h"
#include "brave/components/ai_chat/core/browser/model_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"

namespace ai_chat {

inline ModelService* GetModelServiceForBrowserTest(
    BrowserWindowInterface* browser) {
  return ModelServiceFactory::GetForBrowserContext(browser->GetProfile());
}

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_AI_CHAT_AI_CHAT_BROWSERTEST_UTIL_H_
