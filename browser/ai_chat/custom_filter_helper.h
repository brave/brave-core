// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_AI_CHAT_CUSTOM_FILTER_HELPER_H_
#define BRAVE_BROWSER_AI_CHAT_CUSTOM_FILTER_HELPER_H_

#include <string>

#include "base/functional/callback_forward.h"

class PrefService;

namespace ai_chat {

// Callback for filter creation result
using CreateCustomFilterCallback =
    base::OnceCallback<void(bool success, const std::string& error)>;

// Creates a custom cosmetic filter from AI-generated markdown response.
// Parses the response, validates the code, creates the scriptlet resource
// (if applicable), and adds the filter rule to Brave Shields AdBlock.
//
// |profile_prefs| - Profile preferences for accessing AdBlock services
// |markdown_response| - AI's markdown response containing filter code
// |domain| - Domain to apply the filter to
// |callback| - Called with success status and error message
void CreateCustomFilterImpl(PrefService* profile_prefs,
                            const std::string& markdown_response,
                            const std::string& domain,
                            CreateCustomFilterCallback callback);

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_AI_CHAT_CUSTOM_FILTER_HELPER_H_
