// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/common/features.h"
#include "brave/content/browser/devtools/ai_chat_handler.h"

#define BRAVE_BROWSER_DEVTOOLS_AGENT_HOST_ATTACH_SESSION              \
  if (ai_chat::features::IsAIChatCDPEnabled()) {                     \
    session->CreateAndAddHandler<brave::devtools::AIChatHandler>(this); \
  }

#include <content/browser/devtools/browser_devtools_agent_host.cc>

#undef BRAVE_BROWSER_DEVTOOLS_AGENT_HOST_ATTACH_SESSION
