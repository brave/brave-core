/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "build/build_config.h"  // For OS_MAC
#include "chrome/browser/search/search.h"
#include "content/public/browser/browser_url_handler.h"
#include "extensions/buildflags/buildflags.h"
#include "ui/gfx/image/image_skia.h"

#if defined(OS_MAC)
#include "brave/browser/brave_browser_main_parts_mac.h"
#undef ChromeBrowserMainPartsMac
#define ChromeBrowserMainPartsMac BraveBrowserMainPartsMac
#endif

#if !BUILDFLAG(ENABLE_EXTENSIONS)
#define CHROME_BROWSER_WEB_APPLICATIONS_POLICY_WEB_APP_POLICY_MANAGER_H_
// NOLINTNEXTLINE
#define CHROME_BROWSER_WEB_APPLICATIONS_SYSTEM_WEB_APPS_SYSTEM_WEB_APP_MANAGER_H_
#define CHROME_BROWSER_WEB_APPLICATIONS_WEB_APP_PROVIDER_H_
#define CHROME_BROWSER_WEB_APPLICATIONS_WEB_APP_REGISTRAR_H_
#endif  // BUILDFLAG(ENABLE_EXTENSIONS)

#define HandleNewTabURLRewrite HandleNewTabURLRewrite_ChromiumImpl
#define HandleNewTabURLReverseRewrite HandleNewTabURLReverseRewrite_ChromiumImpl

namespace search {
bool HandleNewTabURLRewrite(GURL* url, content::BrowserContext* bc) {
  return false;
}
bool HandleNewTabURLReverseRewrite(GURL* url, content::BrowserContext* bc) {
  return false;
}
}  // namespace search

#include "src/chrome/browser/chrome_content_browser_client.cc"

#undef HandleNewTabURLRewrite
#undef HandleNewTabURLReverseRewrite_ChromiumImpl
