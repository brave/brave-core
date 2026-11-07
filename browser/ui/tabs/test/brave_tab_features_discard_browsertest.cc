// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/tabs/test/brave_tab_features_discard_browsertest.h"

#include "chrome/browser/resource_coordinator/lifecycle_unit_state.mojom.h"
#include "chrome/browser/resource_coordinator/tab_lifecycle_unit_external.h"
#include "chrome/browser/resource_coordinator/tab_lifecycle_unit_source.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/test_navigation_observer.h"
#include "content/public/test/test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "ui/base/window_open_disposition.h"

BraveTabFeaturesDiscardBrowserTest::BraveTabFeaturesDiscardBrowserTest() =
    default;

BraveTabFeaturesDiscardBrowserTest::~BraveTabFeaturesDiscardBrowserTest() =
    default;

void BraveTabFeaturesDiscardBrowserTest::SetUpCommandLine(
    base::CommandLine* command_line) {
  InProcessBrowserTest::SetUpCommandLine(command_line);
  mock_cert_verifier_.SetUpCommandLine(command_line);
}

void BraveTabFeaturesDiscardBrowserTest::SetUpInProcessBrowserTestFixture() {
  InProcessBrowserTest::SetUpInProcessBrowserTestFixture();
  mock_cert_verifier_.SetUpInProcessBrowserTestFixture();
}

void BraveTabFeaturesDiscardBrowserTest::TearDownInProcessBrowserTestFixture() {
  mock_cert_verifier_.TearDownInProcessBrowserTestFixture();
  InProcessBrowserTest::TearDownInProcessBrowserTestFixture();
}

void BraveTabFeaturesDiscardBrowserTest::SetUpOnMainThread() {
  InProcessBrowserTest::SetUpOnMainThread();
  mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
  host_resolver()->AddRule("*", "127.0.0.1");
  https_server_.ServeFilesFromSourceDirectory(GetChromeTestDataDir());
  ASSERT_TRUE(https_server_.Start());
}

GURL BraveTabFeaturesDiscardBrowserTest::GetURL(std::string_view path) {
  return https_server_.GetURL("a.com", path);
}

tabs::TabInterface* BraveTabFeaturesDiscardBrowserTest::OpenBackgroundTab(
    const GURL& url,
    base::Location location) {
  SCOPED_TRACE(location.ToString());
  EXPECT_TRUE(ui_test_utils::NavigateToURLWithDisposition(
      browser(), url, WindowOpenDisposition::NEW_BACKGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP));
  TabStripModel* tab_strip_model = browser()->GetTabStripModel();
  return tab_strip_model->GetTabAtIndex(tab_strip_model->count() - 1);
}

void BraveTabFeaturesDiscardBrowserTest::DiscardAndReload(
    tabs::TabInterface* tab,
    base::Location location) {
  SCOPED_TRACE(location.ToString());
  content::WebContents* old_contents = tab->GetContents();
  content::WebContentsDestroyedWatcher destroyed_watcher(old_contents);
  auto* lifecycle_unit =
      resource_coordinator::TabLifecycleUnitSource::GetTabLifecycleUnitExternal(
          old_contents);
  ASSERT_TRUE(lifecycle_unit);
  ASSERT_TRUE(
      lifecycle_unit->DiscardTab(mojom::LifecycleUnitDiscardReason::URGENT));
  destroyed_watcher.Wait();

  // Reload the new contents the way TabLifecycleUnit::Load() does. Activating
  // the tab would also reload it, but only once it becomes visible, which is
  // not reliable for test windows on every platform.
  content::WebContents* new_contents = tab->GetContents();
  content::TestNavigationObserver reload_observer(new_contents);
  new_contents->GetController().SetNeedsReload();
  new_contents->GetController().LoadIfNecessary();
  reload_observer.Wait();
}
