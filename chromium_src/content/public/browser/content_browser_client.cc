/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "content/public/browser/content_browser_client.h"

#include "../../../../../content/public/browser/content_browser_client.cc"

namespace content {

std::string ContentBrowserClient::GetEffectiveUserAgent(
    BrowserContext* browser_context,
    const GURL& url) {
  return std::string();
}

}  // namespace content
