/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/modules/clipboard/clipboard_promise.h"

#include "third_party/blink/renderer/core/clipboard/system_clipboard.h"

#define WritePlainText(...)  \
  SanitizeOnNextWriteText(); \
  system_clipboard->WritePlainText(__VA_ARGS__)

#include "src/third_party/blink/renderer/modules/clipboard/clipboard_promise.cc"

#undef WritePlainText
