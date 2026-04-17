// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "base/strings/strcat.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/ui/commander/commander_service.h"
#include "brave/browser/ui/commander/commander_service_factory.h"
#include "brave/components/commander/common/constants.h"
#include "brave/components/commander/common/features.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/location_bar/location_bar.h"
#include "chrome/browser/ui/omnibox/omnibox_view.h"
#include "chrome/test/interaction/interactive_browser_test.h"
#include "content/public/test/browser_test.h"
#include "ui/base/interaction/state_observer.h"

class CommanderServiceInteractiveUITest : public InteractiveBrowserTest {
 public:
  CommanderServiceInteractiveUITest() {
    features_.InitAndEnableFeature(features::kBraveCommander);
  }
  ~CommanderServiceInteractiveUITest() override = default;

  void TearDownOnMainThread() override {
    commander()->Hide();
    InteractiveBrowserTest::TearDownOnMainThread();
  }

 protected:
  commander::CommanderService* commander() {
    return commander::CommanderServiceFactory::GetForBrowserContext(
        browser()->profile());
  }

  OmniboxView* omnibox() {
    return browser()->window()->GetLocationBar()->GetOmniboxView();
  }

  auto WaitForShowing() {
    DEFINE_LOCAL_STATE_IDENTIFIER_VALUE(ui::test::PollingStateObserver<bool>,
                                        kCommanderShowing);
    return Steps(PollState(kCommanderShowing,
                           [this] { return commander()->IsShowing(); }),
                 WaitForState(kCommanderShowing, true),
                 StopObservingState(kCommanderShowing));
  }

  auto WaitForHidden() {
    DEFINE_LOCAL_STATE_IDENTIFIER_VALUE(ui::test::PollingStateObserver<bool>,
                                        kCommanderHidden);
    return Steps(PollState(kCommanderHidden,
                           [this] { return !commander()->IsShowing(); }),
                 WaitForState(kCommanderHidden, true),
                 StopObservingState(kCommanderHidden));
  }

 private:
  base::test::ScopedFeatureList features_;
};

IN_PROC_BROWSER_TEST_F(CommanderServiceInteractiveUITest, HideClearsText) {
  RunTestSequence(
      Do([this] { commander()->Show(); }), Do([this] {
        omnibox()->SetUserText(
            base::StrCat({commander::kCommandPrefix, u" Hello World"}));
      }),
      Do([this] { commander()->Hide(); }), WaitForHidden(),
      CheckResult([this] { return omnibox()->GetText(); }, u"about:blank"));
}

IN_PROC_BROWSER_TEST_F(CommanderServiceInteractiveUITest,
                       CanHideCommanderViaText) {
  RunTestSequence(
      Do([this] {
        omnibox()->SetUserText(
            base::StrCat({commander::kCommandPrefix, u" Hello World"}));
      }),
      WaitForShowing(), Do([this] { omnibox()->SetUserText(u"Hello World"); }),
      WaitForHidden());
}
