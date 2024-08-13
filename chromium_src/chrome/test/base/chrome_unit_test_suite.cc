/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/test/base/chrome_unit_test_suite.h"

#include "brave/common/brave_content_client.h"
#include "brave/common/resource_bundle_helper.h"
#include "brave/test/base/testing_brave_browser_process.h"
#include "ui/base/resource/resource_bundle.h"

#define InitSharedInstanceWithLocale(...)    \
  InitSharedInstanceWithLocale(__VA_ARGS__); \
  brave::InitializeResourceBundle();

#define ChromeContentClient BraveContentClient
#include "src/chrome/test/base/chrome_unit_test_suite.cc"
#undef ChromeContentClient
#undef InitSharedInstanceWithLocale
