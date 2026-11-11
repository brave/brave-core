/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "chrome/browser/search/search.h"
#include "content/public/browser/browser_url_handler.h"
#include "content/public/common/url_constants.h"
#include "extensions/buildflags/buildflags.h"
#include "ui/gfx/image/image_skia.h"
#include "url/origin.h"

#if BUILDFLAG(ENABLE_AI_CHAT)
#include "brave/components/ai_chat/core/common/leo_workspace_util.h"
#endif  // BUILDFLAG(ENABLE_AI_CHAT)

#if !BUILDFLAG(ENABLE_EXTENSIONS)
// CHROMIUM_SRC_NOLINT
#define CHROME_BROWSER_WEB_APPLICATIONS_POLICY_WEB_APP_POLICY_MANAGER_H_
// CHROMIUM_SRC_NOLINT
#define CHROME_BROWSER_WEB_APPLICATIONS_SYSTEM_WEB_APPS_SYSTEM_WEB_APP_MANAGER_H_
// CHROMIUM_SRC_NOLINT
#define CHROME_BROWSER_WEB_APPLICATIONS_WEB_APP_PROVIDER_H_
// CHROMIUM_SRC_NOLINT
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

namespace {

// Leo workspace origins always use first-party storage keys, so they see the
// same storage (e.g. the IndexedDB-persisted directory handle and the viewer's
// service worker registration) whether loaded top-level or embedded in an
// iframe.
bool BraveShouldUseFirstPartyStorageKey(const url::Origin& origin) {
#if BUILDFLAG(ENABLE_AI_CHAT)
  return origin.scheme() == content::kChromeUIUntrustedScheme &&
         (ai_chat::IsAIChatLeoWorkspaceHost(origin.host()) ||
          ai_chat::IsAIChatLeoWorkspaceViewHost(origin.host()));
#else
  return false;
#endif  // BUILDFLAG(ENABLE_AI_CHAT)
}

}  // namespace

#include <chrome/browser/chrome_content_browser_client.cc>
#undef HandleNewTabURLRewrite
#undef HandleNewTabURLReverseRewrite
#if !BUILDFLAG(ENABLE_EXTENSIONS)
#undef CHROME_BROWSER_WEB_APPLICATIONS_WEB_APP_REGISTRAR_H_
#undef CHROME_BROWSER_WEB_APPLICATIONS_WEB_APP_PROVIDER_H_
#undef CHROME_BROWSER_WEB_APPLICATIONS_SYSTEM_WEB_APPS_SYSTEM_WEB_APP_MANAGER_H_
#endif  // BUILDFLAG(ENABLE_EXTENSIONS)
