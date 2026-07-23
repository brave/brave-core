/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <content/public/browser/webui_config.cc>

namespace content {

bool WebUIConfig::ShouldHandleSubdomains() const {
  return false;
}

}  // namespace content
