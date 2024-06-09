/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_content_browser_client.h"
#include "brave/common/brave_content_client.h"
#include "brave/renderer/brave_content_renderer_client.h"

#define ChromeContentClient() BraveContentClient()
#define ChromeContentBrowserClient() BraveContentBrowserClient()
#define ChromeContentRendererClient() BraveContentRendererClient()
#include "src/chrome/test/base/chrome_render_view_test.cc"
#undef ChromeContentClient
#undef ChromeContentBrowserClient
#undef ChromeContentRendererClient
