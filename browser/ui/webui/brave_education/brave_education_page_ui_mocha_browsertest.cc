/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/constants/webui_url_constants.h"
#include "chrome/test/base/web_ui_mocha_browser_test.h"
#include "content/public/test/browser_test.h"

class BraveEducationAppTest : public WebUIMochaBrowserTest {
 protected:
  BraveEducationAppTest() { set_test_loader_host(kBraveGettingStartedHost); }
};

IN_PROC_BROWSER_TEST_F(BraveEducationAppTest, All) {
  RunTest("brave/brave_education/brave_education_app_test.js", "mocha.run()");
}
