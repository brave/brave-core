// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_DREAMING_TEXT_UTILS_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_DREAMING_TEXT_UTILS_H_

#include <string>
#include <string_view>
#include <vector>

#include "base/time/time.h"
#include "brave/components/local_ai/buildflags/buildflags.h"

static_assert(BUILDFLAG(ENABLE_LOCAL_AI));

namespace ai_chat {

// Splits |text| into sentences, with the ICU sentence rules. Each sentence has
// no whitespace at its start and end. Empty sentences are removed.
std::vector<std::string> SplitIntoSentences(std::string_view text);

// Returns true when |sentence| contains data that Leo never stores: an email
// address, a phone number, or an ID or account number.
bool HasDeniedPattern(std::string_view sentence);

// The max length of a memory text, in UTF-16 code units.
inline constexpr size_t kMaxMemoryTextLength = 512;

// Checks text that the LLM wrote from |sources|. Returns true when the text
// is not too long, and each number and each name in it also appears in
// |sources|. The year, month and day of |dates|, and month and day names, are
// also allowed. A name is a word with a capital letter that is not the first
// word.
bool IsFaithfulRewrite(std::string_view text,
                       const std::vector<std::string>& sources,
                       const std::vector<base::Time>& dates);

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_DREAMING_TEXT_UTILS_H_
