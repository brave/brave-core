/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/chrome_browser_main.h"
#include "brave/browser/brave_browser_process_impl.h"
#if !defined(OS_ANDROID)
#include "brave/browser/ui/webui/brave_untrusted_web_ui_controller_factory.h"
#include "chrome/browser/ui/webui/chrome_untrusted_web_ui_controller_factory.h"
#endif

#define BrowserProcessImpl BraveBrowserProcessImpl
#if !defined(OS_ANDROID)
#define ChromeUntrustedWebUIControllerFactory               \
  BraveUntrustedWebUIControllerFactory::RegisterInstance(); \
  ChromeUntrustedWebUIControllerFactory
#endif
#include "../../../../chrome/browser/chrome_browser_main.cc"
#if !defined(OS_ANDROID)
#undef ChromeUntrustedWebUIControllerFactory
#endif
#undef BrowserProcessImpl
