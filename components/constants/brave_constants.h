/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_CONSTANTS_BRAVE_CONSTANTS_H_
#define BRAVE_COMPONENTS_CONSTANTS_BRAVE_CONSTANTS_H_

#include "base/files/file_path.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"

namespace brave {

extern const base::FilePath::CharType kSessionProfileDir[];

#if BUILDFLAG(ENABLE_BRAVE_AI_CHAT_AGENT_PROFILE)
// Path to create the AI Chat agent profile
extern const base::FilePath::CharType kAIChatAgentProfileDir[];
#endif

}  // namespace brave

#endif  // BRAVE_COMPONENTS_CONSTANTS_BRAVE_CONSTANTS_H_
