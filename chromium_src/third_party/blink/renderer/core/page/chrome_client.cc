/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "src/third_party/blink/renderer/core/page/chrome_client.cc"

namespace blink {

const display::ScreenInfos& ChromeClient::BraveGetScreenInfos(
    LocalFrame& frame) const {
  return this->GetScreenInfos(frame);
}

}  // namespace blink
