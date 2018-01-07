/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/app/brave_main_delegate.h"

#include "base/lazy_instance.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "brave/browser/brave_content_browser_client.h"
#include "brave/common/resource_bundle_helper.h"
#include "brave/renderer/brave_content_renderer_client.h"
#include "brave/utility/brave_content_utility_client.h"

#if !defined(CHROME_MULTIPLE_DLL_BROWSER)
base::LazyInstance<BraveContentRendererClient>::DestructorAtExit
    g_brave_content_renderer_client = LAZY_INSTANCE_INITIALIZER;
base::LazyInstance<BraveContentUtilityClient>::DestructorAtExit
    g_brave_content_utility_client = LAZY_INSTANCE_INITIALIZER;
#endif
#if !defined(CHROME_MULTIPLE_DLL_CHILD)
base::LazyInstance<BraveContentBrowserClient>::DestructorAtExit
    g_brave_content_browser_client = LAZY_INSTANCE_INITIALIZER;
#endif

BraveMainDelegate::BraveMainDelegate()
    : ChromeMainDelegate() {}

BraveMainDelegate::BraveMainDelegate(base::TimeTicks exe_entry_point_ticks)
    : ChromeMainDelegate(exe_entry_point_ticks) {}

BraveMainDelegate::~BraveMainDelegate() {}

content::ContentBrowserClient*
BraveMainDelegate::CreateContentBrowserClient() {
#if defined(CHROME_MULTIPLE_DLL_CHILD)
  return NULL;
#else
  return g_brave_content_browser_client.Pointer();
#endif
}

content::ContentRendererClient*
BraveMainDelegate::CreateContentRendererClient() {
#if defined(CHROME_MULTIPLE_DLL_BROWSER)
  return NULL;
#else
  return g_brave_content_renderer_client.Pointer();
#endif
}

content::ContentUtilityClient*
BraveMainDelegate::CreateContentUtilityClient() {
#if defined(CHROME_MULTIPLE_DLL_BROWSER)
  return NULL;
#else
  return g_brave_content_utility_client.Pointer();
#endif
}

bool BraveMainDelegate::ShouldEnableProfilerRecording() {
  return false;
}

void BraveMainDelegate::PreSandboxStartup() {
  ChromeMainDelegate::PreSandboxStartup();
  if (brave::SubprocessNeedsResourceBundle()) {
    brave::InitializeResourceBundle();
  }
}
