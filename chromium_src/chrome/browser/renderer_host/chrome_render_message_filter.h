/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_RENDERER_HOST_CHROME_RENDER_MESSAGE_FILTER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_RENDERER_HOST_CHROME_RENDER_MESSAGE_FILTER_H_

#include "brave/components/content_settings/core/browser/brave_cookie_settings.h"

#define CookieSettings BraveCookieSettings
#include "../../../../../../chrome/browser/renderer_host/chrome_render_message_filter.h"
#undef CookieSettings

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_RENDERER_HOST_CHROME_RENDER_MESSAGE_FILTER_H_
