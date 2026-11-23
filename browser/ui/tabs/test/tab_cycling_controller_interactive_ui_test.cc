/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/tabs/public/tab_cycling_controller.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/interactive_test_utils.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"
#include "ui/aura/window.h"
#include "ui/base/base_window.h"
#include "ui/events/event_constants.h"
#include "ui/events/keycodes/keyboard_codes.h"
#include "ui/events/test/event_generator.h"

class TabCyclingControllerInteractiveUITest : public InProcessBrowserTest {
 public:
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    browser()->GetProfile()->GetPrefs()->SetBoolean(prefs::kCtrlTabMru, true);
    chrome::NewTab(browser(), NewTabTypes::kNewTabCommand);
    ASSERT_TRUE(ui_test_utils::BringBrowserWindowToFront(browser()));
  }

  TabCyclingController& controller() {
    return *TabCyclingController::From(browser());
  }

  ui::test::EventGenerator CreateEventGenerator() {
    aura::Window* window = browser()->GetWindow()->GetNativeWindow();
    return ui::test::EventGenerator(window->GetRootWindow(), window);
  }
};

IN_PROC_BROWSER_TEST_F(TabCyclingControllerInteractiveUITest,
                       EndsOnCtrlRelease) {
  auto generator = CreateEventGenerator();

  chrome::ExecuteCommand(browser(), IDC_CYCLE_TO_NEXT_TAB);
  EXPECT_TRUE(controller().is_cycling());
  generator.ReleaseKey(ui::VKEY_CONTROL, ui::EF_NONE);
  EXPECT_FALSE(controller().is_cycling());
}

IN_PROC_BROWSER_TEST_F(TabCyclingControllerInteractiveUITest,
                       EndsOnMousePress) {
  auto generator = CreateEventGenerator();

  chrome::ExecuteCommand(browser(), IDC_CYCLE_TO_NEXT_TAB);
  EXPECT_TRUE(controller().is_cycling());
  generator.PressLeftButton();
  EXPECT_FALSE(controller().is_cycling());
}
