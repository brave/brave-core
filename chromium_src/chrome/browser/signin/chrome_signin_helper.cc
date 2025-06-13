/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Disable execution path wich previously was under
// IsEnabled(kVerifyRequestInitiatorForMirrorHeaders). Now this feature is
// removed at upstream and defaulted to true. It is false at Brave.
#define BRAVE_PROCESS_MIRROR_HEADERS_IF_FALSE if (false)

#include "src/chrome/browser/signin/chrome_signin_helper.cc"

#undef BRAVE_PROCESS_MIRROR_HEADERS_IF_FALSE
