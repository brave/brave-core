// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/commander/commander_service.h"

#include <memory>
#include <string>

#include "base/functional/callback_forward.h"
#include "base/location.h"
#include "base/run_loop.h"
#include "base/strings/strcat.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "brave/browser/ui/commander/commander_service_factory.h"
#include "brave/components/commander/common/constants.h"
#include "brave/components/commander/common/features.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/location_bar/location_bar.h"
#include "chrome/browser/ui/omnibox/omnibox_view.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/grit/generated_resources.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/grit/brave_components_strings.h"
#include "content/public/test/browser_test.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "ui/base/l10n/l10n_util.h"

class CommanderServiceBrowserTest : public InProcessBrowserTest {
 public:
  CommanderServiceBrowserTest() {
    features_.InitAndEnableFeature(features::kBraveCommander);
  }

  ~CommanderServiceBrowserTest() override = default;
  void SetUp() override { InProcessBrowserTest::SetUp(); }
  void TearDownOnMainThread() override {
    commander()->Hide();
    WaitUntil(base::BindLambdaForTesting(
        [this] { return !commander()->IsShowing(); }));
  }

 protected:
  Profile* profile() { return browser()->profile(); }
  commander::CommanderService* commander() {
    return commander::CommanderServiceFactory::GetForBrowserContext(profile());
  }

  OmniboxView* omnibox() {
    return browser()->window()->GetLocationBar()->GetOmniboxView();
  }

  void WaitUntil(base::RepeatingCallback<bool()> condition) {
    if (condition.Run()) {
      return;
    }

    base::RepeatingTimer scheduler;
    scheduler.Start(FROM_HERE, base::Milliseconds(100),
                    base::BindLambdaForTesting([this, &condition] {
                      if (condition.Run()) {
                        run_loop_->Quit();
                      }
                    }));
    Run();
  }

 private:
  void Run() {
    run_loop_ = std::make_unique<base::RunLoop>();
    run_loop_->Run();
  }

  base::test::ScopedFeatureList features_;
  std::unique_ptr<base::RunLoop> run_loop_;
};

IN_PROC_BROWSER_TEST_F(CommanderServiceBrowserTest, CanShowCommander) {
  EXPECT_FALSE(commander()->IsShowing());

  commander()->Show();
  WaitUntil(
      base::BindLambdaForTesting([&]() { return commander()->IsShowing(); }));
}

IN_PROC_BROWSER_TEST_F(CommanderServiceBrowserTest,
                       CanShowCommanderViaOmniboxText) {
  EXPECT_FALSE(commander()->IsShowing());

  omnibox()->SetUserText(
      base::StrCat({commander::kCommandPrefix, u" Hello World"}));
  WaitUntil(
      base::BindLambdaForTesting([&]() { return commander()->IsShowing(); }));
}

IN_PROC_BROWSER_TEST_F(CommanderServiceBrowserTest, CanHideCommander) {
  commander()->Show();
  WaitUntil(
      base::BindLambdaForTesting([&]() { return commander()->IsShowing(); }));

  commander()->Hide();
  WaitUntil(
      base::BindLambdaForTesting([&]() { return !commander()->IsShowing(); }));
}

// NOTE: This test will pass in isolation but they depend on focus
// so they'll fail if run with other tests. It'd be a good candidate for an
// interactive UI test.
IN_PROC_BROWSER_TEST_F(CommanderServiceBrowserTest, MANUAL_HideClearsText) {
  commander()->Show();
  omnibox()->SetUserText(
      base::StrCat({commander::kCommandPrefix, u" Hello World"}));

  commander()->Hide();
  WaitUntil(
      base::BindLambdaForTesting([&]() { return !commander()->IsShowing(); }));
  EXPECT_EQ(u"about:blank", omnibox()->GetText());
}

// NOTE: This test will pass in isolation but they depend on focus
// so they'll fail if run with other tests. It'd be a good candidate for an
// interactive UI test.
IN_PROC_BROWSER_TEST_F(CommanderServiceBrowserTest,
                       MANUAL_CanHideCommanderViaText) {
  omnibox()->SetUserText(
      base::StrCat({commander::kCommandPrefix, u" Hello World"}));
  WaitUntil(
      base::BindLambdaForTesting([&]() { return commander()->IsShowing(); }));

  omnibox()->SetUserText(u"Hello World");
  WaitUntil(
      base::BindLambdaForTesting([&]() { return !commander()->IsShowing(); }));
}

IN_PROC_BROWSER_TEST_F(CommanderServiceBrowserTest,
                       CommandsAreUpdatedViaOmnibox) {
  omnibox()->SetUserText(
      base::StrCat({commander::kCommandPrefix, u" NT Right"}));

  // Wait for commander to process the input and update results
  WaitUntil(base::BindLambdaForTesting([&]() {
    return commander()->GetResultSetId() >= 1 &&
           commander()->GetItems().size() == 1u;
  }));

  auto items = commander()->GetItems();
  ASSERT_EQ(1u, items.size());
  EXPECT_EQ(l10n_util::GetStringUTF16(IDS_TAB_CXMENU_NEWTABTORIGHT),
            items[0].title);
}

IN_PROC_BROWSER_TEST_F(CommanderServiceBrowserTest, CommandsCanBeSelected) {
  omnibox()->SetUserText(
      base::StrCat({commander::kCommandPrefix, u" New tab"}));

  // Wait for commander to process the input and update results
  WaitUntil(base::BindLambdaForTesting([&]() {
    return commander()->GetResultSetId() >= 1 &&
           commander()->GetItems().size() == 4u;
  }));

  auto items = commander()->GetItems();
  ASSERT_EQ(4u, items.size());
  // For localized IDS_NEW_TAB string, remove & accelerator
  std::u16string expected_new_tab = l10n_util::GetStringUTF16(IDS_NEW_TAB);
  std::u16string::size_type pos = expected_new_tab.find(u"&");
  if (pos != std::u16string::npos) {
    expected_new_tab.erase(pos, 1);
  }
  EXPECT_EQ(expected_new_tab, items[0].title);
  EXPECT_EQ(l10n_util::GetStringUTF16(IDS_TAB_CXMENU_NEWTABTORIGHT),
            items[1].title);

  EXPECT_EQ(1, browser()->tab_strip_model()->count());
  commander()->SelectCommand(0, commander()->GetResultSetId());
  EXPECT_EQ(2, browser()->tab_strip_model()->count());
}

IN_PROC_BROWSER_TEST_F(CommanderServiceBrowserTest,
                       CompositeCommandsCanBeSelected) {
  omnibox()->SetUserText(
      base::StrCat({commander::kCommandPrefix, u" ",
                    l10n_util::GetStringUTF16(IDS_IDC_WINDOW_PIN_TAB)}));

  // Wait for commander to process the input and update results
  WaitUntil(base::BindLambdaForTesting([&]() {
    return commander()->GetResultSetId() >= 1 &&
           commander()->GetItems().size() == 3u;
  }));

  auto items = commander()->GetItems();
  ASSERT_EQ(3u, items.size());
  EXPECT_EQ(l10n_util::GetStringUTF16(IDS_IDC_WINDOW_PIN_TAB), items[0].title);
  EXPECT_EQ(l10n_util::GetStringUTF16(IDS_COMMANDER_PIN_TAB), items[1].title);
  EXPECT_EQ(l10n_util::GetStringUTF16(IDS_IDC_WINDOW_CLOSE_UNPINNED_TABS),
            items[2].title);

  commander()->SelectCommand(1, 1);
  EXPECT_LE(2, commander()->GetResultSetId());

  // This is retriggered on a different thread normally, but we want to force it
  // here because otherwise the tests get a bit flakey with focus.
  omnibox()->SetUserText(commander::kCommandPrefix.data());

  items = commander()->GetItems();
  ASSERT_GE(items.size(), 1u);  // At least one item should be available

  // Find "about:blank" in the results (might not be first on all platforms)
  size_t about_blank_index = 0;
  bool found_about_blank = false;
  for (size_t i = 0; i < items.size(); ++i) {
    if (items[i].title == u"about:blank") {
      about_blank_index = i;
      found_about_blank = true;
      break;
    }
  }
  EXPECT_TRUE(found_about_blank)
      << "Should find 'about:blank' command in results";

  EXPECT_FALSE(browser()->tab_strip_model()->IsTabPinned(0));
  commander()->SelectCommand(about_blank_index, commander()->GetResultSetId());
  EXPECT_TRUE(browser()->tab_strip_model()->IsTabPinned(0));
}
