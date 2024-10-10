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
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/grit/brave_components_strings.h"
#include "components/omnibox/browser/omnibox_view.h"
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

// This test is flaky on macOS CI.
#if BUILDFLAG(IS_MAC)
#define MAYBE_CommandsAreUpdatedViaOmnibox DISABLED_CommandsAreUpdatedViaOmnibox
#else
#define MAYBE_CommandsAreUpdatedViaOmnibox CommandsAreUpdatedViaOmnibox
#endif
IN_PROC_BROWSER_TEST_F(CommanderServiceBrowserTest,
                       MAYBE_CommandsAreUpdatedViaOmnibox) {
  omnibox()->SetUserText(
      base::StrCat({commander::kCommandPrefix, u" NT Right"}));

  EXPECT_LE(1, commander()->GetResultSetId());

  auto items = commander()->GetItems();
  ASSERT_EQ(1u, items.size());
#if BUILDFLAG(IS_MAC)
  EXPECT_EQ(u"New Tab to the Right", items[0].title);
#else
  EXPECT_EQ(u"New tab to the right", items[0].title);
#endif
}

// This test is flaky on macOS CI.
#if BUILDFLAG(IS_MAC)
#define MAYBE_CommandsCanBeSelected DISABLED_CommandsCanBeSelected
#else
#define MAYBE_CommandsCanBeSelected CommandsCanBeSelected
#endif
IN_PROC_BROWSER_TEST_F(CommanderServiceBrowserTest,
                       MAYBE_CommandsCanBeSelected) {
  omnibox()->SetUserText(
      base::StrCat({commander::kCommandPrefix, u" New tab"}));

  EXPECT_LE(1, commander()->GetResultSetId());

  auto items = commander()->GetItems();
  ASSERT_EQ(2u, items.size());
  EXPECT_EQ(u"New tab", items[0].title);
#if BUILDFLAG(IS_MAC)
  EXPECT_EQ(u"New Tab to the Right", items[1].title);
#else
  EXPECT_EQ(u"New tab to the right", items[1].title);
#endif

  EXPECT_EQ(1, browser()->tab_strip_model()->count());
  commander()->SelectCommand(0, commander()->GetResultSetId());
  EXPECT_EQ(2, browser()->tab_strip_model()->count());
}

// This test is flaky on macOS CI.
#if BUILDFLAG(IS_MAC)
#define MAYBE_CompositeCommandsCanBeSelected \
  DISABLED_CompositeCommandsCanBeSelected
#else
#define MAYBE_CompositeCommandsCanBeSelected CompositeCommandsCanBeSelected
#endif
IN_PROC_BROWSER_TEST_F(CommanderServiceBrowserTest,
                       MAYBE_CompositeCommandsCanBeSelected) {
  omnibox()->SetUserText(
      base::StrCat({commander::kCommandPrefix, u" ",
                    l10n_util::GetStringUTF16(IDS_IDC_WINDOW_PIN_TAB)}));

  EXPECT_LE(1, commander()->GetResultSetId());

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
  ASSERT_EQ(1u, items.size());
  EXPECT_EQ(u"about:blank", items[0].title);

  EXPECT_FALSE(browser()->tab_strip_model()->IsTabPinned(0));
  commander()->SelectCommand(0, commander()->GetResultSetId());
  EXPECT_TRUE(browser()->tab_strip_model()->IsTabPinned(0));
}
