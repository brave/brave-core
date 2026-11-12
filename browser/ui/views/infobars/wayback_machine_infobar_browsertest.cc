// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/infobars/wayback_machine_infobar.h"

#include "base/functional/bind.h"
#include "base/strings/strcat.h"
#include "base/test/run_until.h"
#include "base/timer/timer.h"
#include "brave/browser/infobars/wayback_machine_infobar_delegate.h"
#include "brave/components/brave_wayback_machine/brave_wayback_machine_tab_helper.h"
#include "brave/components/brave_wayback_machine/url_constants.h"
#include "brave/components/brave_wayback_machine/wayback_state.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/infobars/content/content_infobar_manager.h"
#include "components/infobars/core/infobar.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/test_navigation_observer.h"
#include "content/public/test/url_loader_interceptor.h"
#include "ui/events/base_event_utils.h"
#include "ui/events/event.h"
#include "ui/events/types/event_type.h"
#include "ui/gfx/animation/animation.h"
#include "ui/gfx/animation/animation_test_api.h"
#include "ui/gfx/geometry/point.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/test/button_test_api.h"
#include "url/gurl.h"

namespace {

constexpr char kSnapshotURL[] =
    "https://web.archive.org/web/20240226123456/https://example.com/";

}  // namespace

class WaybackMachineInfoBarBrowserTest : public InProcessBrowserTest {
 protected:
  content::WebContents* active_contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  BraveWaybackMachineTabHelper* GetTabHelper() {
    return BraveWaybackMachineTabHelper::From(
        tabs::TabInterface::MaybeGetFromContents(active_contents()));
  }

  infobars::ContentInfoBarManager* infobar_manager() {
    return infobars::ContentInfoBarManager::FromWebContents(active_contents());
  }

  WaybackMachineInfoBar* GetInfoBar() {
    for (infobars::InfoBar* infobar : infobar_manager()->infobars()) {
      if (infobar->delegate()->GetIdentifier() ==
          infobars::InfoBarDelegate::BRAVE_WAYBACK_MACHINE_INFOBAR_DELEGATE) {
        return static_cast<WaybackMachineInfoBar*>(infobar);
      }
    }
    return nullptr;
  }

  WaybackMachineInfoBarDelegate* GetDelegate() {
    auto* infobar = GetInfoBar();
    return infobar ? static_cast<WaybackMachineInfoBarDelegate*>(
                         infobar->delegate())
                   : nullptr;
  }

  void FetchSnapshotAndShowInfoBar() {
    GetTabHelper()->FetchSnapshotInfo();
    ASSERT_TRUE(base::test::RunUntil([&] {
      return GetTabHelper()->wayback_state() == WaybackState::kFound;
    }));
    ASSERT_TRUE(GetTabHelper()->snapshot_info());
    WaybackMachineInfoBarDelegate::Create(active_contents(),
                                          *GetTabHelper()->snapshot_info());
    ASSERT_NE(GetInfoBar(), nullptr);
  }

  void ClickButton(views::Button* button) {
    views::test::ButtonTestApi(button).NotifyClick(
        ui::MouseEvent(ui::EventType::kMousePressed, gfx::Point(), gfx::Point(),
                       ui::EventTimeForNow(), ui::EF_LEFT_MOUSE_BUTTON, 0));
  }

 private:
  gfx::AnimationTestApi::RenderModeResetter disable_rich_animations_ =
      gfx::AnimationTestApi::SetRichAnimationRenderMode(
          gfx::Animation::RichAnimationRenderMode::FORCE_DISABLED);
  content::URLLoaderInterceptor interceptor_{base::BindRepeating(
      [](content::URLLoaderInterceptor::RequestParams* params) {
        const GURL& url = params->url_request.url;
        if (url.host() == GURL(kWaybackQueryURL).host()) {
          content::URLLoaderInterceptor::WriteResponse(
              "HTTP/1.1 200 OK\nContent-Type: application/json\n\n",
              base::StrCat({R"({"archived_snapshots":{"closest":{"url":")",
                            kSnapshotURL,
                            R"(","timestamp":"20240226123456"}}})"}),
              params->client.get());
          return true;
        }
        if (url == GURL(kSnapshotURL)) {
          content::URLLoaderInterceptor::WriteResponse(
              "HTTP/1.1 200 OK\nContent-Type: text/html\n\n",
              "<html>Snapshot</html>", params->client.get());
          return true;
        }
        return false;
      })};
};

IN_PROC_BROWSER_TEST_F(WaybackMachineInfoBarBrowserTest,
                       CountdownNavigatesToSnapshot) {
  FetchSnapshotAndShowInfoBar();

  content::TestNavigationObserver observer(active_contents());
  for (int i = 0; i < WaybackMachineInfoBarDelegate::kCountdownSeconds; ++i) {
    ASSERT_NE(GetDelegate(), nullptr);
    ASSERT_TRUE(GetDelegate()->countdown_timer_for_testing().IsRunning());
    GetDelegate()->countdown_timer_for_testing().FireNow();
  }
  observer.Wait();

  EXPECT_EQ(active_contents()->GetLastCommittedURL(), GURL(kSnapshotURL));
  EXPECT_EQ(GetTabHelper()->wayback_state(), WaybackState::kLoaded);
  EXPECT_EQ(GetInfoBar(), nullptr);
}

IN_PROC_BROWSER_TEST_F(WaybackMachineInfoBarBrowserTest,
                       RedirectButtonNavigatesImmediately) {
  FetchSnapshotAndShowInfoBar();

  content::TestNavigationObserver observer(active_contents());
  ClickButton(GetInfoBar()->redirect_button_for_testing());
  observer.Wait();

  EXPECT_EQ(active_contents()->GetLastCommittedURL(), GURL(kSnapshotURL));
  EXPECT_EQ(GetTabHelper()->wayback_state(), WaybackState::kLoaded);
  EXPECT_EQ(GetInfoBar(), nullptr);
}

IN_PROC_BROWSER_TEST_F(WaybackMachineInfoBarBrowserTest,
                       CancelButtonKeepsFoundStateWithoutNavigating) {
  FetchSnapshotAndShowInfoBar();

  ClickButton(GetInfoBar()->cancel_button_for_testing());

  EXPECT_EQ(GetInfoBar(), nullptr);
  EXPECT_EQ(GetTabHelper()->wayback_state(), WaybackState::kFound);
}
