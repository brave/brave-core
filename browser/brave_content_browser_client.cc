/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_content_browser_client.h"

#include "brave/browser/brave_browser_main_extra_parts.h"

BraveContentBrowserClient::BraveContentBrowserClient() {}

BraveContentBrowserClient::~BraveContentBrowserClient() {}

content::BrowserMainParts* BraveContentBrowserClient::CreateBrowserMainParts(
      const content::MainFunctionParams& parameters) {
  ChromeBrowserMainParts* main_parts = (ChromeBrowserMainParts*)
      ChromeContentBrowserClient::CreateBrowserMainParts(parameters);
  main_parts->AddParts(new BraveBrowserMainExtraParts());
  return main_parts;
}
