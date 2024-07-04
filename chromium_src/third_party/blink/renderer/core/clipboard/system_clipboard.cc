/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/core/clipboard/system_clipboard.h"

#include "src/third_party/blink/renderer/core/clipboard/system_clipboard.cc"

namespace blink {

void SystemClipboard::SanitizeOnNextWriteText() {
  DCHECK(!snapshot_);
  if (!clipboard_.is_bound()) {
    return;
  }
  clipboard_->SanitizeOnNextWriteText();
}

}  // namespace blink
