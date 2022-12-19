/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/web_discovery/web_discovery_tab_helper.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/infobars/content/content_infobar_manager.h"
#include "components/infobars/core/infobar_manager.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "testing/gmock/include/gmock/gmock.h"

using WebDiscoveryTest = InProcessBrowserTest;
using ::testing::_;

namespace {

class TestObserver : public infobars::InfoBarManager::Observer {
 public:
  TestObserver() = default;
  ~TestObserver() override = default;
  MOCK_METHOD(void, OnInfoBarAdded, (infobars::InfoBar * infobar), (override));
};

}  // namespace

IN_PROC_BROWSER_TEST_F(WebDiscoveryTest, InfobarAddedTest) {
  auto* model = browser()->tab_strip_model();
  auto* contents = model->GetActiveWebContents();
  auto* tab_helper = WebDiscoveryTabHelper::FromWebContents(contents);
  auto* infobar_manager =
      infobars::ContentInfoBarManager::FromWebContents(contents);

  TestObserver observer;
  EXPECT_CALL(observer, OnInfoBarAdded(_)).Times(1);
  infobar_manager->AddObserver(&observer);
  tab_helper->ShowInfoBar(browser()->profile()->GetPrefs());
  infobar_manager->RemoveObserver(&observer);
}
