// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_AI_CHAT_CUSTOM_FILTER_HELPER_H_
#define BRAVE_BROWSER_AI_CHAT_CUSTOM_FILTER_HELPER_H_

#include <string>

#include "base/functional/callback.h"

class PrefService;

namespace ai_chat {

// Callback type for CreateCustomFilter completion
using CreateCustomFilterCallback =
    base::OnceCallback<void(bool success, const std::string& error_message)>;

// Creates a custom scriptlet and filter rule from AI-generated markdown response.
// This function parses the markdown, creates the scriptlet file in AdBlock's
// custom resource provider, and adds the filter rule to custom filters.
//
// @param profile_prefs The profile's pref service (needed for AdBlock APIs)
// @param markdown_response The markdown-formatted response from the AI containing:
//                          - JavaScript code block
//                          - Scriptlet name block
//                          - Filter rule block
// @param domain The domain this filter applies to
// @param callback Called with success status and error message (empty on success)
void CreateCustomFilterImpl(PrefService* profile_prefs,
                            const std::string& markdown_response,
                            const std::string& domain,
                            CreateCustomFilterCallback callback);

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_AI_CHAT_CUSTOM_FILTER_HELPER_H_
