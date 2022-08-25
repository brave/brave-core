/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "src/content/public/browser/content_browser_client.cc"

namespace content {

std::string ContentBrowserClient::GetEffectiveUserAgent(
    BrowserContext* browser_context,
    const GURL& url) {
  return std::string();
}

bool ContentBrowserClient::AllowWorkerFingerprinting(
    const GURL& url,
    BrowserContext* browser_context) {
  return true;
}

uint8_t ContentBrowserClient::WorkerGetBraveFarblingLevel(
    const GURL& url,
    BrowserContext* browser_context) {
  return 1 /* OFF */;
}

}  // namespace content
