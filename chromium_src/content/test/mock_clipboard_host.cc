/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "content/test/mock_clipboard_host.h"

#include <content/test/mock_clipboard_host.cc>

namespace content {

void MockClipboardHost::SanitizeOnNextWriteText() {}

}  // namespace content
