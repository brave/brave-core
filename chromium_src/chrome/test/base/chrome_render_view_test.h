/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef CHROMIUM_SRC_CHROME_TEST_BASE_CHROME_RENDER_VIEW_TEST_H_
#define CHROMIUM_SRC_CHROME_TEST_BASE_CHROME_RENDER_VIEW_TEST_H_

#include "chrome/renderer/chrome_content_renderer_client.h"
#define ChromeContentRendererClient BraveContentRendererClient
class BraveContentRendererClient;
#include "src/chrome/test/base/chrome_render_view_test.h"
#undef ChromeContentRendererClient

#endif  // CHROMIUM_SRC_CHROME_TEST_BASE_CHROME_RENDER_VIEW_TEST_H_
