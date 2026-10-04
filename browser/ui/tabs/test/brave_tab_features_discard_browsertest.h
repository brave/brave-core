// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_TABS_TEST_BRAVE_TAB_FEATURES_DISCARD_BROWSERTEST_H_
#define BRAVE_BROWSER_UI_TABS_TEST_BRAVE_TAB_FEATURES_DISCARD_BROWSERTEST_H_

#include <string_view>

#include "base/location.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "url/gurl.h"

namespace tabs {
class TabInterface;
}  // namespace tabs

// On desktop, discarding a tab replaces its WebContents. Tests built on this
// fixture check that BraveTabFeatures members keep working on the new contents.
// Each feature's tests live in their own file, built only when the feature is.
class BraveTabFeaturesDiscardBrowserTest : public InProcessBrowserTest {
 public:
  BraveTabFeaturesDiscardBrowserTest();
  ~BraveTabFeaturesDiscardBrowserTest() override;

  // InProcessBrowserTest:
  void SetUpCommandLine(base::CommandLine* command_line) override;
  void SetUpInProcessBrowserTestFixture() override;
  void TearDownInProcessBrowserTestFixture() override;
  void SetUpOnMainThread() override;

 protected:
  // Returns `path` on the HTTPS test server, on host a.com.
  GURL GetURL(std::string_view path);

  // Opens `url` in a new background tab and returns it.
  tabs::TabInterface* OpenBackgroundTab(
      const GURL& url,
      base::Location location = base::Location::Current());

  // Discards the background `tab`, which replaces its WebContents, then
  // reloads the page into the new contents. The tab stays in the background.
  void DiscardAndReload(tabs::TabInterface* tab,
                        base::Location location = base::Location::Current());

 private:
  content::ContentMockCertVerifier mock_cert_verifier_;
  net::EmbeddedTestServer https_server_{net::EmbeddedTestServer::TYPE_HTTPS};
};

#endif  // BRAVE_BROWSER_UI_TABS_TEST_BRAVE_TAB_FEATURES_DISCARD_BROWSERTEST_H_
