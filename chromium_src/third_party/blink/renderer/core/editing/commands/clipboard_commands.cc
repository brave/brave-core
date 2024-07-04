/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/core/editing/commands/clipboard_commands.h"

// Sanitize data only if there's an API call.
#define BRAVE_CLIPBOARD_COMMANDS_CHECK_SOURCE              \
  if (source == EditorCommandSource::kDOM) {               \
    frame.GetSystemClipboard()->SanitizeOnNextWriteText(); \
  }

#include "src/third_party/blink/renderer/core/editing/commands/clipboard_commands.cc"

#undef BRAVE_CLIPBOARD_COMMANDS_CHECK_SOURCE
