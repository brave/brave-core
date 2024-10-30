/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __RE___
#define __RE___

#include "third_party/blink/renderer/core/inspector/devtools_agent.h"

#define AttachSession(...)                 \
  AttachSession_ChromiumImpl(__VA_ARGS__); \
  void AttachSession(__VA_ARGS__)

#include "src/third_party/blink/renderer/core/exported/web_dev_tools_agent_impl.h"

#undef AttachSession

#endif
