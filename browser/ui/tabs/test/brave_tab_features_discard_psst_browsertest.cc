// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "base/functional/callback_helpers.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/psst/psst_ui_desktop_presenter.h"
#include "brave/browser/ui/tabs/public/brave_tab_features.h"
#include "brave/browser/ui/tabs/test/brave_tab_features_discard_browsertest.h"
#include "brave/browser/ui/views/page_action/psst_action_controller.h"
#include "brave/components/psst/core/common/features.h"
#include "components/infobars/content/content_infobar_manager.h"
#include "components/infobars/core/infobar.h"
#include "components/infobars/core/infobar_delegate.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/test/browser_test.h"

class BraveTabFeaturesDiscardPsstBrowserTest
    : public BraveTabFeaturesDiscardBrowserTest {
 public:
  BraveTabFeaturesDiscardPsstBrowserTest() {
    scoped_feature_list_.InitAndEnableFeature(psst::features::kEnablePsst);
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_F(BraveTabFeaturesDiscardPsstBrowserTest,
                       InfoBarShownInDiscardedTab) {
  tabs::TabInterface* tab = OpenBackgroundTab(GetURL("/title2.html"));
  auto* psst_action_controller =
      tabs::BraveTabFeatures::FromTabFeatures(tab->GetTabFeatures())
          ->psst_page_action_controller();
  ASSERT_TRUE(psst_action_controller);
  psst::PsstUiDesktopPresenter presenter(*tab,
                                         psst_action_controller->AsWeakPtr());

  ASSERT_NO_FATAL_FAILURE(DiscardAndReload(tab));

  presenter.ShowInfoBar(base::DoNothing());
  auto* infobar_manager =
      infobars::ContentInfoBarManager::FromWebContents(tab->GetContents());
  ASSERT_TRUE(infobar_manager);
  ASSERT_EQ(1u, infobar_manager->infobars().size());
  EXPECT_EQ(infobars::InfoBarDelegate::BRAVE_PSST_INFOBAR_DELEGATE,
            infobar_manager->infobars()[0]->GetIdentifier());
}
