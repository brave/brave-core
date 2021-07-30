/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/test/base/testing_browser_process.h"

#include "brave/test/base/testing_brave_browser_process.h"

#define TestingBrowserProcess TestingBrowserProcess_ChromiumImpl
#include "../../../../../chrome/test/base/testing_browser_process.cc"
#undef TestingBrowserProcess

// static
void TestingBrowserProcess::CreateInstance() {
  TestingBrowserProcess_ChromiumImpl::CreateInstance();
  TestingBraveBrowserProcess::CreateInstance();
}

// static
void TestingBrowserProcess::DeleteInstance() {
  TestingBrowserProcess_ChromiumImpl::DeleteInstance();
  TestingBraveBrowserProcess::DeleteInstance();
}

// static
TestingBrowserProcess* TestingBrowserProcess::GetGlobal() {
  return static_cast<TestingBrowserProcess*>(
      TestingBrowserProcess_ChromiumImpl::GetGlobal());
}
