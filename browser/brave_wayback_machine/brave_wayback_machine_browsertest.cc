/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wayback_machine/brave_wayback_machine_util.h"
#include "chrome/browser/infobars/infobar_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/infobars/core/infobar_manager.h"
#include "content/public/browser/web_contents.h"
#include "testing/gmock/include/gmock/gmock.h"

using BraveWaybackMachineTest = InProcessBrowserTest;
using ::testing::_;

namespace {

class TestObserver : public infobars::InfoBarManager::Observer {
 public:
  TestObserver() = default;
  ~TestObserver() override = default;
  MOCK_METHOD1(OnInfoBarAdded, void(infobars::InfoBar* infobar));
};

}  // namespace

IN_PROC_BROWSER_TEST_F(BraveWaybackMachineTest, InfobarAddTest) {
  EXPECT_TRUE(IsWaybackMachineEnabled(browser()->profile()));

  auto* model = browser()->tab_strip_model();
  auto* contents = model->GetActiveWebContents();
  InfoBarService* infobar_service = InfoBarService::FromWebContents(contents);

  // For 200 response code, wayback inforbar isn't added.
  TestObserver observer_for_200;
  EXPECT_CALL(observer_for_200, OnInfoBarAdded(_)).Times(0);
  infobar_service->AddObserver(&observer_for_200);
  CheckWaybackMachineIfNeeded(contents, 200);
  infobar_service->RemoveObserver(&observer_for_200);

  // For 404 response code, wayback inforbar is added.
  TestObserver observer_for_404;
  EXPECT_CALL(observer_for_404, OnInfoBarAdded(_)).Times(1);
  infobar_service->AddObserver(&observer_for_404);
  CheckWaybackMachineIfNeeded(contents, 404);
  infobar_service->RemoveObserver(&observer_for_404);
}
