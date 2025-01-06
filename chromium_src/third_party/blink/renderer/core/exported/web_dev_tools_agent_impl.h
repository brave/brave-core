// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_EXPORTED_WEB_DEV_TOOLS_AGENT_IMPL_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_EXPORTED_WEB_DEV_TOOLS_AGENT_IMPL_H_

#include "third_party/blink/renderer/core/inspector/devtools_agent.h"

#define AttachSession(...)                 \
  AttachSession_ChromiumImpl(__VA_ARGS__); \
  void AttachSession(__VA_ARGS__)

#include "src/third_party/blink/renderer/core/exported/web_dev_tools_agent_impl.h"  // IWYU pragma: export

#undef AttachSession

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_EXPORTED_WEB_DEV_TOOLS_AGENT_IMPL_H_
