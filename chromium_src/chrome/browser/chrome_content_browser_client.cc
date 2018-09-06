/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "build/build_config.h"  // For OS_MACOSX

#if defined(OS_MACOSX)
#include "brave/browser/brave_browser_main_parts_mac.h"
#undef ChromeBrowserMainPartsMac
#define ChromeBrowserMainPartsMac BraveBrowserMainPartsMac
#endif

#if defined(OS_LINUX)
#include "brave/browser/ui/views/brave_browser_main_extra_parts_views_linux.h"
#define ChromeBrowserMainExtraPartsViewsLinux BraveBrowserMainExtraPartsViewsLinux
#endif

#include "../../../../chrome/browser/chrome_content_browser_client.cc"

#if defined(OS_LINUX)
#undef ChromeBrowserMainExtraPartsViewsLinux
#endif