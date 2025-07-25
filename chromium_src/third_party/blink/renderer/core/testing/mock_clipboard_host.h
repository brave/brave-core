/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_TESTING_MOCK_CLIPBOARD_HOST_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_TESTING_MOCK_CLIPBOARD_HOST_H_

#include "third_party/blink/public/mojom/clipboard/clipboard.mojom-blink.h"

#define CommitWrite       \
  CommitWrite() override; \
  void SanitizeOnNextWriteText

#include <third_party/blink/renderer/core/testing/mock_clipboard_host.h>  // IWYU pragma: export

#undef CommitWrite

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_TESTING_MOCK_CLIPBOARD_HOST_H_
