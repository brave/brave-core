/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_UTILS_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_UTILS_H_

#include "base/functional/callback_forward.h"
#include "brave/components/text_recognition/common/buildflags/buildflags.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "url/gurl.h"

class PrefService;

namespace ai_chat {

// Check both policy and feature flag to determine if AI Chat is enabled.
bool IsAIChatEnabled(PrefService* prefs);
bool HasUserOptedIn(PrefService* prefs);
void SetUserOptedIn(PrefService* prefs, bool opted_in);

bool IsBraveSearchSERP(const GURL& url);

#if BUILDFLAG(ENABLE_TEXT_RECOGNITION)
using GetOCRTextCallback = base::OnceCallback<void(std::string)>;
void GetOCRText(const SkBitmap& image, GetOCRTextCallback callback);
#endif

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_UTILS_H_
