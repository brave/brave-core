/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "build/build_config.h"  // For OS_MACOSX
#include "brave/components/brave_shields/browser/buildflags/buildflags.h"  // For STP
#include "chrome/browser/search/search.h"
#include "content/public/browser/browser_url_handler.h"

#if BUILDFLAG(BRAVE_STP_ENABLED)
#include "brave/browser/renderer_host/brave_render_message_filter.h"
#undef ChromeRenderMessageFilter
#define ChromeRenderMessageFilter BraveRenderMessageFilter
#endif

#if defined(OS_MACOSX)
#include "brave/browser/brave_browser_main_parts_mac.h"
#undef ChromeBrowserMainPartsMac
#define ChromeBrowserMainPartsMac BraveBrowserMainPartsMac
#endif

#if defined(OS_LINUX)
#include "brave/browser/ui/views/brave_browser_main_extra_parts_views_linux.h"
#define ChromeBrowserMainExtraPartsViewsLinux
    BraveBrowserMainExtraPartsViewsLinux
#endif

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

#include "../../../../chrome/browser/chrome_content_browser_client.cc"  // NOLINT

#undef HandleNewTabURLRewrite
#undef HandleNewTabURLReverseRewrite_ChromiumImpl

#if defined(OS_LINUX)
#undef ChromeBrowserMainExtraPartsViewsLinux
#endif
