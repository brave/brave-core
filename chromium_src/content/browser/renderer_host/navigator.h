/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_RENDERER_HOST_NAVIGATOR_H_
#define BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_RENDERER_HOST_NAVIGATOR_H_

#include "content/browser/renderer_host/navigation_controller_impl.h"

#define NavigationControllerImpl BraveNavigationControllerImpl
#include "src/content/browser/renderer_host/navigator.h"
#undef NavigationControllerImpl

#endif  // BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_RENDERER_HOST_NAVIGATOR_H_
