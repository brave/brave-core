/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "headless/lib/browser/headless_client_hints_controller_delegate.h"

#include "src/headless/lib/browser/headless_client_hints_controller_delegate.cc"

namespace headless {

blink::UserAgentMetadata
HeadlessClientHintsControllerDelegate::BraveGetUserAgentMetadata(
    GURL top_url) {
  return GetUserAgentMetadata();
}

}  // namespace headless
